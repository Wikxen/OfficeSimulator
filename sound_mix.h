#ifndef OFFICE_SOUND_MIX_H
#define OFFICE_SOUND_MIX_H

#include "sim.h"

enum { SOUND_RATE = 44100, SOUND_LOOP_FRAMES = SOUND_RATE * 96 / 5,
    SOUND_VOICES = 4 };

typedef enum {
    SOUND_FURNITURE, SOUND_TECH, SOUND_AMENITIES, SOUND_HIRE, SOUND_MOVE,
    SOUND_PROFIT, SOUND_LOSS, SOUND_QUIT, SOUND_NEW, SOUND_COUNT
} SoundEffect;

typedef struct {
    unsigned frame, next_voice;
    struct {
        SoundEffect effect;
        unsigned frame;
        bool active;
    } voices[SOUND_VOICES];
} SoundMix;

void sound_mix_init(void);
void sound_mix_play(SoundMix *mix, SoundEffect effect);
void sound_mix_render(SoundMix *mix, int16_t *out, size_t frames, int staff, bool audible);

#endif
