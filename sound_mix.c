#include "sound_mix.h"
#include <math.h>
#include <string.h>

enum { BEAT = SOUND_RATE * 3 / 5, EFFECT_FRAMES = SOUND_RATE * 3 / 5 };
static int16_t music[SOUND_LOOP_FRAMES], office[SOUND_LOOP_FRAMES];
static int16_t effects[SOUND_COUNT][EFFECT_FRAMES];
static const double tau = 6.283185307179586;

static void add(int16_t *sample, int value)
{
    int total = *sample + value;
    *sample = total > INT16_MAX ? INT16_MAX : total < INT16_MIN ? INT16_MIN : (int16_t)total;
}

static void note(int16_t *track, unsigned length, unsigned start, unsigned count,
    double hz, double end_hz, int gain, double overtone)
{
    double phase = 0, step = tau * hz / SOUND_RATE;
    double drift = tau * (end_hz - hz) / (SOUND_RATE * (double)count);
    for (unsigned i = 0, at = start; i < count; ++i) {
        double attack = i < 220 ? i / 220.0 : 1.0;
        double tail = 1.0 - i / (double)count;
        add(&track[at], (int)(gain * attack * tail * tail * (sin(phase) + overtone*sin(phase*2))));
        phase += step;
        step += drift;
        if (++at == length) at = 0;
    }
}

static uint32_t noise_next(uint32_t *state)
{
    uint32_t n = *state;
    n ^= n << 13;
    n ^= n >> 17;
    n ^= n << 5;
    *state = n;
    return n;
}

static void texture(int16_t *track, unsigned length, unsigned start, unsigned count, int gain, double smoothing)
{
    uint32_t seed = UINT32_C(0xff104ddb) + start;
    double slow = 0;
    for (unsigned i = 0, at = start; i < count; ++i) {
        double noise = (int)(noise_next(&seed) >> 16) / 32768.0 - 1.0;
        slow += (noise - slow)*smoothing;
        double tail = 1.0 - i / (double)count;
        add(&track[at], (int)(gain * slow * tail * tail * (i < 48 ? i/48.0 : 1.0)));
        if (++at == length) at = 0;
    }
}

static void music_loop(void)
{
    static const int chords[][4] = {
        {48, 64, 67, 71}, {45, 60, 64, 67}, {41, 60, 64, 69}, {43, 62, 65, 69},
        {48, 62, 64, 67}, {45, 60, 64, 71}, {41, 60, 65, 69}, {43, 62, 67, 71},
    };
    static const int melody[][4] = {
        {76, 79, 74, 71}, {72, 76, 79, 76}, {77, 76, 72, 69}, {74, 77, 72, 71},
        {67, 72, 74, 76}, {72, 76, 71, 67}, {69, 72, 77, 76}, {74, 71, 69, 67},
    };
    for (unsigned bar = 0; bar < 8; ++bar) {
        unsigned start = bar*4*BEAT;
        for (int n = 0; n < 4; ++n) {
            double hz = 440 * pow(2, (chords[bar][n] - 69)/12.0);
            note(music, SOUND_LOOP_FRAMES, start + n*180, BEAT*3, hz, hz, n ? 430 : 740, .13);
            hz = 440 * pow(2, (melody[bar][n] - 69)/12.0);
            unsigned phrase = start + (n*2 + (n == 1))*BEAT/2;
            note(music, SOUND_LOOP_FRAMES, phrase, BEAT*5/4, hz, hz, n == 3 ? 610 : 780, .26);
            note(music, SOUND_LOOP_FRAMES, (phrase + BEAT*3/4) % SOUND_LOOP_FRAMES,
                BEAT, hz, hz, 145, .15);
        }
    }
}

static void office_loop(void)
{
    for (unsigned i = 0; i < SOUND_LOOP_FRAMES; ++i) {
        double seconds = i/(double)SOUND_RATE;
        office[i] = (int16_t)((105 + 16*sin(tau*seconds/4.8))*sin(tau*90*seconds)
            + 28*sin(tau*180*seconds));
    }
    uint32_t seed = UINT32_C(0x430cfba3);
    for (unsigned beat = 0; beat < 32; ++beat) {
        unsigned start = beat*BEAT + noise_next(&seed) % (BEAT/2);
        unsigned hits = 1 + noise_next(&seed)%4;
        for (unsigned i = 0; i < hits; ++i) {
            unsigned at = (start + i*3100) % SOUND_LOOP_FRAMES;
            texture(office, SOUND_LOOP_FRAMES, at, 970, 580 + (int)(noise_next(&seed)%325), .68);
            if (i == 0) note(office, SOUND_LOOP_FRAMES, at, 900, 590, 370, 210, .4);
        }
        if (beat % 7 == 3) texture(office, SOUND_LOOP_FRAMES, start, SOUND_RATE/3, 1250, .13);
    }
}

static void chime(SoundEffect effect, unsigned delay_ms, unsigned length_ms,
    double hz, double end_hz, int gain)
{
    note(effects[effect], EFFECT_FRAMES, SOUND_RATE*delay_ms/1000, SOUND_RATE*length_ms/1000,
        hz, end_hz, gain, .24);
}

static void rustle(SoundEffect effect, unsigned delay_ms, unsigned length_ms, int gain, double smoothing)
{
    texture(effects[effect], EFFECT_FRAMES, SOUND_RATE*delay_ms/1000, SOUND_RATE*length_ms/1000,
        gain, smoothing);
}

void sound_mix_init(void)
{
    static bool initialized;
    if (initialized) return;
    initialized = true;
    music_loop();
    office_loop();
    chime(SOUND_FURNITURE, 0, 115, 175, 85, 3900);
    chime(SOUND_FURNITURE, 115, 95, 220, 100, 2900);
    rustle(SOUND_FURNITURE, 0, 90, 2800, .48);
    chime(SOUND_TECH, 0, 140, 740, 920, 2600);
    chime(SOUND_TECH, 105, 235, 1175, 1175, 2250);
    rustle(SOUND_AMENITIES, 0, 350, 2800, .26);
    chime(SOUND_AMENITIES, 155, 210, 1050, 880, 2700);
    chime(SOUND_HIRE, 0, 220, 523, 523, 2800);
    chime(SOUND_HIRE, 125, 380, 784, 784, 2800);
    for (unsigned i = 0; i < 3; ++i) {
        rustle(SOUND_MOVE, 105*i, 165, 2800 + (int)i*400, .23);
        chime(SOUND_MOVE, 105*i, 160, 115, 75, 1700);
    }
    chime(SOUND_PROFIT, 0, 180, 784, 784, 2800);
    chime(SOUND_PROFIT, 85, 230, 988, 988, 2700);
    chime(SOUND_PROFIT, 170, 390, 1319, 1319, 2600);
    chime(SOUND_LOSS, 0, 220, 294, 247, 2950);
    chime(SOUND_LOSS, 160, 350, 220, 196, 2750);
    rustle(SOUND_QUIT, 0, 260, 2600, .38);
    chime(SOUND_QUIT, 125, 300, 392, 262, 2900);
    chime(SOUND_NEW, 0, 340, 392, 392, 2550);
    chime(SOUND_NEW, 180, 400, 659, 659, 2450);
}

void sound_mix_play(SoundMix *mix, SoundEffect effect)
{
    unsigned i = mix->next_voice;
    for (unsigned n = 0; n < SOUND_VOICES; ++n) {
        unsigned slot = (i + n) % SOUND_VOICES;
        if (!mix->voices[slot].active) {
            i = slot;
            break;
        }
    }
    mix->voices[i].active = true;
    mix->voices[i].effect = effect;
    mix->voices[i].frame = 0;
    mix->next_voice = (i + 1) % SOUND_VOICES;
}

void sound_mix_render(SoundMix *mix, int16_t *out, size_t frames, int staff, bool audible)
{
    if (!audible) {
        memset(out, 0, frames * sizeof *out);
        memset(mix->voices, 0, sizeof mix->voices);
        return;
    }
    int bustle = staff < 8 ? staff : 8;
    for (size_t i = 0; i < frames; ++i) {
        unsigned frame = mix->frame;
        int value = music[frame] + office[frame]*(4 + bustle)/5;
        for (unsigned v = 0; v < SOUND_VOICES; ++v) {
            if (!mix->voices[v].active) continue;
            value += effects[mix->voices[v].effect][mix->voices[v].frame++];
            if (mix->voices[v].frame == EFFECT_FRAMES) mix->voices[v].active = false;
        }
        out[i] = 0;
        add(&out[i], value);
        if (++mix->frame == SOUND_LOOP_FRAMES) mix->frame = 0;
    }
}
