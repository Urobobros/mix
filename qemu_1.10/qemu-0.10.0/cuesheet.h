#ifndef CUESHEET_H
#define CUESHEET_H

typedef struct CueTrack {
    int lba;
    int is_audio;
} CueTrack;

typedef struct CueSheet {
    char bin_path[1024];
    int track_count;
    CueTrack tracks[100];
} CueSheet;

extern CueSheet cue_sheet;

#endif /* CUESHEET_H */
