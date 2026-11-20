#define WIN32_LEAN_AND_MEAN
#include "sound.h"
#include <stdio.h>

enum { BUFFER_FRAMES = SOUND_RATE / 50, BUFFER_COUNT = 4 };
static HWAVEOUT output;
static WAVEHDR headers[BUFFER_COUNT];
static int16_t buffers[BUFFER_COUNT][BUFFER_FRAMES];
static SoundMix mix;

static bool checked(MMRESULT status, const char *operation)
{
    if (status == MMSYSERR_NOERROR) return true;
    char msg[160], error[MAXERRORLENGTH];
    if (waveOutGetErrorTextA(status, error, sizeof error) != MMSYSERR_NOERROR)
        snprintf(error, sizeof error, "error %u", status);
    snprintf(msg, sizeof msg, "Office Simulator audio: %s: %s\n", operation, error);
    OutputDebugStringA(msg);
    return false;
}

static bool enqueue(unsigned index, int staff, bool audible)
{
    sound_mix_render(&mix, buffers[index], BUFFER_FRAMES, staff, audible);
    return checked(waveOutWrite(output, &headers[index], sizeof headers[index]), "queue audio");
}

bool sound_open(HWND window, int staff, bool audible)
{
    WAVEFORMATEX format = {WAVE_FORMAT_PCM, 1, SOUND_RATE, SOUND_RATE*sizeof(int16_t),
        sizeof(int16_t), 16, 0};
    if (!checked(waveOutOpen(&output, WAVE_MAPPER, &format, (DWORD_PTR)window, 0, CALLBACK_WINDOW),
            "open audio device")) {
        output = NULL;
        return false;
    }
    sound_mix_init();
    for (unsigned i = 0; i < BUFFER_COUNT; ++i) {
        headers[i].lpData = (LPSTR)buffers[i];
        headers[i].dwBufferLength = sizeof buffers[i];
        if (!checked(waveOutPrepareHeader(output, &headers[i], sizeof headers[i]), "prepare audio")
            || !enqueue(i, staff, audible)) {
            sound_close();
            return false;
        }
    }
    return true;
}

bool sound_done(WPARAM device, LPARAM header, int staff, bool audible)
{
    if (!output || (HWAVEOUT)device != output) return true;
    for (unsigned i = 0; i < BUFFER_COUNT; ++i) {
        if ((WAVEHDR *)header == &headers[i])
            return enqueue(i, staff, audible);
    }
    OutputDebugStringA("Office Simulator audio: unexpected completed audio buffer.\n");
    return false;
}

void sound_play(SoundEffect effect)
{
    sound_mix_play(&mix, effect);
}

void sound_close(void)
{
    if (!output) return;
    checked(waveOutReset(output), "stop audio");
    for (unsigned i = 0; i < BUFFER_COUNT; ++i) {
        if (headers[i].dwFlags & WHDR_PREPARED)
            checked(waveOutUnprepareHeader(output, &headers[i], sizeof headers[i]), "release audio buffer");
    }
    checked(waveOutClose(output), "close audio device");
    output = NULL;
}
