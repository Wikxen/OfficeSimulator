#ifndef OFFICE_SCENE_H
#define OFFICE_SCENE_H

#include <windows.h>
#include "sim.h"

enum { SCENE_X = 42, SCENE_Y = 259, SCENE_W = 531, SCENE_H = 256, SCENE_CUE_MS = 1500 };
enum { SCENE_WEEK = ACTION_COUNT, SCENE_QUIT, SCENE_NEW };

typedef struct {
    int kind;
    ULONGLONG since;
} SceneCue;

void scene_draw(HDC dc, const Sim *game, ULONGLONG tick, bool motion, const SceneCue *cue);

#endif
