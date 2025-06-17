#ifndef CDAUDIO_H
#define CDAUDIO_H

#include "block.h"
#include "audio/audio.h"

/* Enable verbose CD audio debug output */
//#define DEBUG_CDAUDIO
#ifdef DEBUG_CDAUDIO
# define CDAUDIO_DPRINTF(fmt, ...) \
    fprintf(stderr, "cdaudio: " fmt, ## __VA_ARGS__)
#else
# define CDAUDIO_DPRINTF(fmt, ...) do { } while (0)
#endif

#ifdef CONFIG_CDAUDIO
void cdaudio_init(BlockDriverState *bs);
void cdaudio_set_bs(BlockDriverState *bs);
void cdaudio_play_lba(int start_lba, int nb_sectors);
void cdaudio_pause(int active);
void cdaudio_stop(void);
#else
static inline void cdaudio_init(BlockDriverState *bs) {}
static inline void cdaudio_set_bs(BlockDriverState *bs) {}
static inline void cdaudio_play_lba(int start_lba, int nb_sectors) {}
static inline void cdaudio_pause(int active) {}
static inline void cdaudio_stop(void) {}
#endif

#endif /* CDAUDIO_H */
