#include "hw.h"
#include "cdaudio.h"
#include "bswap.h"

#ifdef CONFIG_CDAUDIO

typedef struct CDAudioState {
    QEMUSoundCard card;
    SWVoiceOut *voice;
    BlockDriverState *bs;
    int playing;
    int cur_lba;
    int end_lba;
} CDAudioState;

static CDAudioState cd_audio;

static void cdaudio_callback(void *opaque, int free)
{
    CDAudioState *s = opaque;
    uint8_t sector[2352];

    CDAUDIO_DPRINTF("callback free=%d play=%d cur=%d end=%d\n", free,
                    s->playing, s->cur_lba, s->end_lba);

    while (free >= 2352 && s->playing && s->cur_lba < s->end_lba) {
        if (!s->bs)
            break;
        CDAUDIO_DPRINTF("read lba=%d\n", s->cur_lba);
        if (bdrv_pread(s->bs, (int64_t)s->cur_lba * 2352, sector, 2352) != 2352)
            break;
#ifndef WORDS_BIGENDIAN
        for (int i = 0; i < 2352; i += 2)
            *(uint16_t *)(sector + i) = bswap16(*(uint16_t *)(sector + i));
#endif
        int copied = AUD_write(s->voice, sector, 2352);
        if (copied < 2352)
            break;
        free -= copied;
        s->cur_lba++;
        if (s->cur_lba >= s->end_lba)
            s->playing = 0;
    }
    if (!s->playing)
        AUD_set_active_out(s->voice, 0);
}

void cdaudio_init(BlockDriverState *bs)
{
    struct audsettings as = {44100, 2, AUD_FMT_S16, AUDIO_HOST_ENDIANNESS};
    AudioState *audio = AUD_init();
    cd_audio.bs = bs;
    AUD_register_card(audio, "cdaudio", &cd_audio.card);
    cd_audio.voice = AUD_open_out(&cd_audio.card, NULL, "cdaudio",
                                  &cd_audio, cdaudio_callback, &as);
    CDAUDIO_DPRINTF("init bs=%p\n", bs);
}

void cdaudio_set_bs(BlockDriverState *bs)
{
    cd_audio.bs = bs;
    CDAUDIO_DPRINTF("set_bs %p\n", bs);
}

void cdaudio_play_lba(int start_lba, int nb_sectors)
{
    if (!cd_audio.voice)
        cdaudio_init(cd_audio.bs);
    CDAUDIO_DPRINTF("play_lba start=%d count=%d\n", start_lba, nb_sectors);
    cd_audio.cur_lba = start_lba;
    cd_audio.end_lba = start_lba + nb_sectors;
    cd_audio.playing = 1;
    AUD_set_active_out(cd_audio.voice, 1);
}

void cdaudio_pause(int active)
{
    if (cd_audio.voice)
        AUD_set_active_out(cd_audio.voice, active);
    CDAUDIO_DPRINTF("pause %d\n", active);
}

void cdaudio_stop(void)
{
    cd_audio.playing = 0;
    CDAUDIO_DPRINTF("stop\n");
    if (cd_audio.voice)
        AUD_set_active_out(cd_audio.voice, 0);
}
#endif /* CONFIG_CDAUDIO */
