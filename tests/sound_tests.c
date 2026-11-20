#include "sound_mix.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { SAMPLE_COUNT = SOUND_RATE * 3 / 5 };
static int16_t background[SAMPLE_COUNT], sample[SAMPLE_COUNT], heard[SOUND_COUNT][SAMPLE_COUNT];

int main(void)
{
    sound_mix_init();
    SoundMix loop = {.frame = SOUND_LOOP_FRAMES - 2};
    sound_mix_render(&loop, sample, 4, 1, true);
    assert(loop.frame == 2 && abs(sample[1] - sample[2]) < 100);

    SoundMix quiet = {0}, busy = {0};
    sound_mix_render(&quiet, background, SAMPLE_COUNT, 1, true);
    sound_mix_render(&busy, sample, SAMPLE_COUNT, STAFF_LIMIT, true);
    assert(memcmp(background, sample, sizeof sample) != 0);

    for (int kind = 0; kind < SOUND_COUNT; ++kind) {
        SoundMix mix = {0};
        sound_mix_play(&mix, (SoundEffect)kind);
        sound_mix_render(&mix, heard[kind], SAMPLE_COUNT, 1, true);
        int peak = 0;
        for (size_t i = 0; i < SAMPLE_COUNT; ++i) {
            int difference = abs(heard[kind][i] - background[i]);
            if (difference > peak) peak = difference;
        }
        assert(peak > 1000 && peak < 9000);
        for (int other = 0; other < kind; ++other)
            assert(memcmp(heard[kind], heard[other], sizeof sample) != 0);
        for (int i = 0; i < SOUND_VOICES; ++i) assert(!mix.voices[i].active);
    }

    SoundMix mix = {0};
    sound_mix_play(&mix, SOUND_HIRE);
    sound_mix_play(&mix, SOUND_TECH);
    sound_mix_render(&mix, sample, SAMPLE_COUNT, 1, true);
    for (size_t i = 0; i < SAMPLE_COUNT; ++i) {
        assert(sample[i] == heard[SOUND_HIRE][i] + heard[SOUND_TECH][i] - background[i]);
    }
    sound_mix_play(&mix, SOUND_MOVE);
    sound_mix_render(&mix, sample, SAMPLE_COUNT, 1, false);
    for (size_t i = 0; i < SAMPLE_COUNT; ++i) assert(sample[i] == 0);
    for (int i = 0; i < SOUND_VOICES; ++i) assert(!mix.voices[i].active);
    assert(mix.frame == SAMPLE_COUNT);
    sound_mix_render(&mix, sample, SAMPLE_COUNT, 1, true);
    sound_mix_render(&quiet, background, SAMPLE_COUNT, 1, true);
    assert(memcmp(background, sample, sizeof sample) == 0);

    puts("Sound tests passed: loop seam, ambience, distinct cues, mixing and mute.");
    return 0;
}
