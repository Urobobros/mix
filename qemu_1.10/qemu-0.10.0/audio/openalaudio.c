/*
 * QEMU OpenAL audio driver based on PCem implementation
 */
#include "qemu-common.h"
#include "audio.h"

#ifdef __APPLE__
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

#define AUDIO_CAP "openal"
#include "audio_int.h"

#define NUM_BUFFERS 4

typedef struct OpenALVoiceOut {
    HWVoiceOut hw;
    ALuint source;
    ALuint buffers[NUM_BUFFERS];
    int queued;
} OpenALVoiceOut;

static ALCdevice *al_dev;
static ALCcontext *al_ctx;

static void *openal_audio_init(void)
{
    al_dev = alcOpenDevice(NULL);
    if (!al_dev) {
        dolog("Could not open OpenAL device\n");
        return NULL;
    }
    al_ctx = alcCreateContext(al_dev, NULL);
    if (!al_ctx || !alcMakeContextCurrent(al_ctx)) {
        dolog("Could not create OpenAL context\n");
        if (al_ctx)
            alcDestroyContext(al_ctx);
        alcCloseDevice(al_dev);
        return NULL;
    }
    return (void*)1;
}

static void openal_audio_fini(void *opaque)
{
    if (al_ctx) {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(al_ctx);
    }
    if (al_dev) {
        alcCloseDevice(al_dev);
    }
}

static int openal_init_out(HWVoiceOut *hw, struct audsettings *as)
{
    OpenALVoiceOut *vo = (OpenALVoiceOut *)hw;
    int i;

    audio_pcm_init_info(&hw->info, as);
    hw->samples = 1024;

    alGenSources(1, &vo->source);
    alGenBuffers(NUM_BUFFERS, vo->buffers);
    vo->queued = 0;

    for (i = 0; i < NUM_BUFFERS; i++) {
        alSourceQueueBuffers(vo->source, 1, &vo->buffers[i]);
        vo->queued++;
    }
    alSourcePlay(vo->source);

    return 0;
}

static void openal_fini_out(HWVoiceOut *hw)
{
    OpenALVoiceOut *vo = (OpenALVoiceOut *)hw;
    alSourceStop(vo->source);
    if (vo->queued) {
        alSourceUnqueueBuffers(vo->source, vo->queued, vo->buffers);
        vo->queued = 0;
    }
    alDeleteSources(1, &vo->source);
    alDeleteBuffers(NUM_BUFFERS, vo->buffers);
}

static int openal_run_out(HWVoiceOut *hw)
{
    OpenALVoiceOut *vo = (OpenALVoiceOut *)hw;
    int processed;
    int live;
    int decr;
    int rpos;
    struct st_sample *src;
    int16_t tmp[1024*2];

    live = audio_pcm_hw_get_live_out(hw);
    if (!live)
        return 0;

    alGetSourcei(vo->source, AL_BUFFERS_PROCESSED, &processed);
    while (processed-- > 0) {
        ALuint buf;
        alSourceUnqueueBuffers(vo->source, 1, &buf);
        vo->queued--;
    }

    decr = live;
    rpos = hw->rpos;
    if (decr > hw->samples)
        decr = hw->samples;

    src = hw->mix_buf + rpos;
    hw->clip(tmp, src, decr);
    alBufferData(vo->buffers[0], AL_FORMAT_STEREO16, tmp, decr << hw->info.shift, hw->info.freq);
    alSourceQueueBuffers(vo->source, 1, &vo->buffers[0]);
    vo->queued++;

    if (vo->queued > NUM_BUFFERS)
        vo->queued = NUM_BUFFERS;

    if (decr)
        alSourcePlay(vo->source);

    hw->rpos = (rpos + decr) % hw->samples;

    return decr;
}

static int openal_write_out(SWVoiceOut *sw, void *buf, int len)
{
    return audio_pcm_sw_write(sw, buf, len);
}

static struct audio_pcm_ops openal_pcm_ops = {
    .init_out  = openal_init_out,
    .fini_out  = openal_fini_out,
    .run_out   = openal_run_out,
    .write     = openal_write_out,
    .ctl_out   = NULL,
    .init_in   = NULL,
    .fini_in   = NULL,
    .run_in    = NULL,
    .read      = NULL,
    .ctl_in    = NULL,
};

struct audio_driver openal_audio_driver = {
    .name           = "openal",
    .descr          = "OpenAL audio output",
    .options        = NULL,
    .init           = openal_audio_init,
    .fini           = openal_audio_fini,
    .pcm_ops        = &openal_pcm_ops,
    .can_be_default = 1,
    .max_voices_out = 1,
    .max_voices_in  = 0,
    .voice_size_out = sizeof(OpenALVoiceOut),
    .voice_size_in  = 0,
};

