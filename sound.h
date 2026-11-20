#ifndef OFFICE_SOUND_H
#define OFFICE_SOUND_H

#include <windows.h>
#include <mmsystem.h>
#include "sound_mix.h"

bool sound_open(HWND window, int staff, bool audible);
bool sound_done(WPARAM device, LPARAM header, int staff, bool audible);
void sound_play(SoundEffect effect);
void sound_close(void);

#endif
