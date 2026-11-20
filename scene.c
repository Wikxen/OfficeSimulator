#define WIN32_LEAN_AND_MEAN
#include "scene.h"
#include <stdio.h>

#define INK RGB(32, 54, 60)
#define CREAM RGB(255, 246, 222)
#define BRASS RGB(229, 167, 80)
#define TEAL RGB(60, 135, 117)
#define LEAF RGB(101, 175, 118)
#define SHADOW RGB(174, 178, 157)
#define GLASS RGB(158, 216, 225)

typedef struct { int x, y; } Spot;

static void rect(HDC dc, int x, int y, int w, int h, COLORREF color)
{
    RECT r = {x, y, x + w, y + h};
    SetDCBrushColor(dc, color);
    FillRect(dc, &r, (HBRUSH)GetStockObject(DC_BRUSH));
}

static void oval(HDC dc, int x, int y, int w, int h, COLORREF color)
{
    SetDCBrushColor(dc, color);
    Ellipse(dc, x, y, x + w, y + h);
}

static void caption(HDC dc, int x, int y, const char *label, COLORREF color)
{
    SetTextColor(dc, color);
    TextOutA(dc, x, y, label, lstrlenA(label));
}

static void plant(HDC dc, int x, int y, COLORREF pot)
{
    oval(dc, x - 11, y + 3, 24, 6, SHADOW);
    rect(dc, x - 5, y - 5, 11, 12, pot);
    rect(dc, x - 7, y - 7, 15, 4, pot);
    rect(dc, x, y - 21, 2, 15, TEAL);
    oval(dc, x - 10, y - 22, 12, 8, TEAL);
    oval(dc, x, y - 25, 10, 10, LEAF);
    oval(dc, x - 5, y - 29, 9, 12, LEAF);
}

static void room(HDC dc, const Sim *s, ULONGLONG tick)
{
    static const COLORREF walls[] = {RGB(181, 207, 194), RGB(172, 202, 196),
        RGB(184, 197, 216), RGB(170, 192, 209)};
    static const COLORREF floors[] = {RGB(225, 214, 190), RGB(224, 206, 178),
        RGB(208, 209, 195), RGB(220, 212, 192)};
    static const int panes[] = {1, 2, 2, 3};
    int tier = s->tier;
    rect(dc, 0, 0, SCENE_W, 65, walls[tier]);
    rect(dc, 0, 65, SCENE_W, SCENE_H - 65, floors[tier]);
    for (int y = 81; y < SCENE_H; y += 14) {
        rect(dc, 0, y, SCENE_W, 1, SHADOW);
        for (int x = y % 3 * 24; x < SCENE_W; x += 73)
            rect(dc, x, y - 13, 1, 13, SHADOW);
    }
    rect(dc, 0, 58, SCENE_W, 7, TEAL);
    rect(dc, 0, 65, 7, SCENE_H - 65, RGB(149, 154, 135));
    rect(dc, SCENE_W - 7, 65, 7, SCENE_H - 65, RGB(149, 154, 135));
    rect(dc, 0, SCENE_H - 6, SCENE_W, 6, INK);

    int window_w = tier == 0 ? 138 : tier == 3 ? 100 : 135;
    for (int i = 0; i < panes[tier]; ++i) {
        int x = 35 + i * (window_w + 14);
        rect(dc, x - 4, 8, window_w + 8, 44, INK);
        rect(dc, x, 12, window_w, 36, GLASS);
        int cloud = (int)((tick / 150 + 19u * i) % (unsigned)(window_w - 28));
        oval(dc, x + cloud, 16, 25, 7, CREAM);
        oval(dc, x + cloud + 5, 13, 12, 10, CREAM);
        for (int j = 0; j < window_w / 14; ++j) {
            int height = 5 + (j * 7 + i * 5 + tier * 4) % 16;
            rect(dc, x + j * 14, 48 - height, 12, height, RGB(101, 153, 170));
            if (height > 13) rect(dc, x + j * 14 + 3, 51 - height, 2, 3, CREAM);
        }
        rect(dc, x + window_w / 2 - 1, 12, 3, 36, INK);
        rect(dc, x - 7, 48, window_w + 14, 4, CREAM);
    }
    oval(dc, 477, 12, 27, 27, INK);
    oval(dc, 480, 15, 21, 21, CREAM);
    static const Spot hands[] = {{0,-7},{5,-5},{7,0},{5,5},{0,7},{-5,5},{-7,0},{-5,-5}};
    Spot h = hands[(tick / 1300) % 8];
    rect(dc, 490, 20, 2, 6, INK);
    for (int i = 1; i <= 5; ++i)
        rect(dc, 490 + h.x*i/5, 25 + h.y*i/5, 2, 2, TEAL);
    rect(dc, 12, 83, 385, 158, RGB(191, 194, 170));
    rect(dc, 16, 87, 377, 150, RGB(232, 230, 209));
    rect(dc, 403, 83, 108, 158, RGB(126, 160, 154));
    rect(dc, 407, 87, 100, 150, RGB(166, 199, 183));
    for (int y = 94; y < 234; y += 8) {
        rect(dc, 410, y, 2, 3, CREAM);
        rect(dc, 502, y, 2, 3, CREAM);
    }
    caption(dc, 421, 90, "BREAK AREA", INK);
}

static Spot station(int tier, int index)
{
    switch (tier) {
    case 0: return (Spot){108 + index*174, 151};
    case 1: return (Spot){78 + (index%3)*126, 130 + (index/3)*68};
    case 2: return (Spot){78 + (index%3)*126, 112 + (index/3)*52};
    default: return (Spot){63 + (index%4)*94, 103 + (index/4)*38};
    }
}

static void technology(HDC dc, Spot p, int level, ULONGLONG tick, int index)
{
    if (!level) return;
    int x = p.x, y = p.y;
    if (level == 1) {
        rect(dc, x - 10, y - 4, 20, 9, INK);
        rect(dc, x - 8, y - 2, 16, 5, GLASS);
        rect(dc, x - 12, y + 5, 24, 2, CREAM);
    } else {
        int w = level >= 3 ? 20 : 26;
        int left = x - (level >= 3 ? 22 : 13);
        for (int i = 0; i < (level >= 3 ? 2 : 1); ++i) {
            int mx = left + i*23;
            rect(dc, mx, y - 9, w, 12, INK);
            rect(dc, mx + 2, y - 7, w - 4, 8, level == 4 ? TEAL : GLASS);
            rect(dc, mx + 4, y - 5, ((tick/280 + (unsigned)(i + index)) % 3 == 0) ? 5 : 10,
                1, level == 4 ? CREAM : TEAL);
            rect(dc, mx + w/2 - 1, y + 3, 3, 3, INK);
        }
        rect(dc, x - 8, y + 6, 16, 2, CREAM);
        if (level == 4) {
            rect(dc, x + 24, y - 5, 7, 12, INK);
            rect(dc, x + 26, y - 3, 3, 2, BRASS);
            rect(dc, x + 26, y + 2, 3, 2, LEAF);
        }
    }
}

static void desk(HDC dc, Spot p, int level, bool occupied, bool has_tech)
{
    static const COLORREF tops[] = {RGB(193, 159, 111), RGB(214, 167, 105),
        RGB(184, 128, 77), RGB(239, 213, 163), RGB(203, 224, 218)};
    static const COLORREF frames[] = {SHADOW, RGB(126, 94, 66), RGB(111, 77, 59),
        RGB(66, 89, 84), INK};
    int x = p.x, y = p.y;
    if (!level && !occupied && !has_tech) {
        rect(dc, x - 17, y + 5, 34, 1, SHADOW);
        rect(dc, x - 3, y + 3, 6, 5, SHADOW);
        return;
    }
    if (!level && !has_tech) return;
    int w = 43 + level*5;
    if (level >= 2) {
        rect(dc, x - w/2 - 4, y + 6, w + 8, 20, level == 2 ? RGB(197, 188, 162)
            : level == 3 ? RGB(170, 196, 176) : RGB(156, 182, 191));
        rect(dc, x - w/2 - 2, y + 23, w + 4, 1, CREAM);
    }
    rect(dc, x - w/2 + 3, y + 8, 4, 8, frames[level]);
    rect(dc, x + w/2 - 7, y + 8, level >= 3 ? 5 : 4, 8, frames[level]);
    rect(dc, x - w/2, y, w, 11, frames[level]);
    rect(dc, x - w/2, y, w, 8, tops[level]);
    if (level >= 2) {
        rect(dc, x - w/2 + 3, y + 8, 9, 7, frames[level]);
        rect(dc, x - w/2 + 5, y + 10, 5, 1, CREAM);
    }
    if (level >= 3) {
        rect(dc, x - w/2 + 5, y - 5, 2, 8, INK);
        rect(dc, x - w/2 + 2, y - 7, 9, 3, level == 3 ? BRASS : TEAL);
    }
    if (level == 4) rect(dc, x - w/2, y + 8, w, 2, TEAL);
    if (level > 0) oval(dc, x - 9, y + 14, 18, 12, level == 1 ? frames[level] : TEAL);
}

static void person(HDC dc, Spot p, int index, int mood, ULONGLONG tick, unsigned hire_age, bool working)
{
    static const COLORREF shirts[] = {BRASS, TEAL, RGB(111, 140, 191), RGB(184, 110, 108),
        RGB(141, 118, 166), RGB(98, 148, 97)};
    static const COLORREF skins[] = {RGB(235, 181, 134), RGB(173, 112, 78),
        RGB(245, 204, 160), RGB(131, 83, 63), RGB(203, 144, 106)};
    static const COLORREF hair[] = {INK, RGB(81, 52, 48), RGB(133, 86, 51), RGB(67, 55, 60)};
    unsigned phase = (unsigned)((tick + (unsigned)index*211) % 2400);
    int bob = tick && phase < 350 ? 1 : 0;
    int hop = (hire_age < 650) ? (int)(hire_age < 220 ? hire_age/22 : (650 - hire_age)/43) : 0;
    int x = p.x, y = p.y + bob - hop;
    COLORREF shirt = shirts[index % 6], skin = skins[index % 5];
    oval(dc, x - 10, p.y + 22, 20, 5, SHADOW);
    rect(dc, x - 7, y + 22, 5, 3, INK);
    rect(dc, x + 2, y + 22, 5, 3, INK);
    oval(dc, x - 9, y + 13, 18, 11, shirt);
    bool typing = working && tick && phase < 1050;
    bool left = phase/120 % 2 != 0;
    rect(dc, x - 9, y + 13 - (typing && left ? 2 : 0), 4, 4, skin);
    rect(dc, x + 5, y + 13 - (typing && !left ? 2 : 0), 4, 4, skin);
    oval(dc, x - 7, y + 3, 14, 13, skin);
    oval(dc, x - 7, y + 1, 14, 7 + index%2, hair[index % 4]);
    int eye_h = tick && phase > 2240 ? 1 : 2;
    rect(dc, x - 4, y + 9, 2, eye_h, INK);
    rect(dc, x + 2, y + 9, 2, eye_h, INK);
    rect(dc, x - 1, y + 13, 3, 1, mood >= 75 ? CREAM : hair[index % 4]);
    if (index == 0) rect(dc, x - 2, y + 18, 4, 2, CREAM);
}

static void lounge(HDC dc, int level, ULONGLONG tick)
{
    if (!level) {
        caption(dc, 429, 153, "Empty", RGB(61, 93, 88));
        return;
    }
    rect(dc, 424, 119, 43, 16, RGB(135, 99, 69));
    rect(dc, 424, 118, 43, 10, CREAM);
    rect(dc, 431, 113, 15, 13, INK);
    rect(dc, 433, 115, 11, 4, RGB(158, 216, 225));
    rect(dc, 448, 120, 6, 6, BRASS);
    if (tick) {
        unsigned puff = (unsigned)(tick/70 % 15);
        if (puff < 11) oval(dc, 450 + (int)(puff%3), 117 - (int)puff, 3, 3, CREAM);
    }
    if (level >= 2) {
        plant(dc, 487, 135, RGB(173, 107, 76));
        plant(dc, 426, 221, BRASS);
    }
    if (level >= 3) {
        rect(dc, 432, 151, 60, 28, RGB(109, 116, 145));
        rect(dc, 434, 151, 56, 9, RGB(141, 151, 181));
        rect(dc, 441, 161, 21, 12, RGB(164, 173, 198));
        rect(dc, 463, 161, 21, 12, RGB(164, 173, 198));
        rect(dc, 487, 158, 6, 17, INK);
        oval(dc, 445, 178, 35, 14, SHADOW);
        oval(dc, 445, 176, 35, 12, CREAM);
    }
    if (level == 4) {
        rect(dc, 466, 200, 30, 25, INK);
        rect(dc, 468, 202, 26, 17, TEAL);
        rect(dc, 480, 202, 1, 17, CREAM);
        rect(dc, 469, 210, 24, 1, CREAM);
        oval(dc, 475 + (int)(tick/120 % 11), 207, 3, 3, BRASS);
    }
}

static void spark(HDC dc, int x, int y, int size, COLORREF color)
{
    rect(dc, x - size, y, size*2 + 1, 2, color);
    rect(dc, x, y - size, 2, size*2 + 1, color);
}

static void feedback(HDC dc, const Sim *s, int kind, unsigned age)
{
    static const char *const labels[] = {"Furniture installed", "Technology online", "New perks unlocked",
        "Welcome aboard!", "Office move complete", "Week complete", "A fresh start"};
    static const Spot rays[] = {{-3,-2},{0,-3},{3,-2},{-4,0},{4,0},{-2,2},{2,2}};
    char msg[56];
    const char *label = labels[kind];
    COLORREF accent = BRASS;
    if (kind == SCENE_WEEK) {
        label = msg;
        snprintf(msg, sizeof msg, "Weekly net: %s$%lld", s->last_net < 0 ? "-" : s->last_net > 0 ? "+" : "",
            s->last_net < 0 ? -s->last_net : s->last_net);
        accent = s->last_net < 0 ? RGB(231, 141, 135) : LEAF;
    } else if (kind == TECH || kind == AMENITIES) accent = GLASS;
    if (kind == MOVE && age < 900) {
        int x = 340 - (int)age/5;
        oval(dc, x - 13, 233, 42, 7, SHADOW);
        rect(dc, x - 14, 228, 42, 4, INK);
        oval(dc, x - 10, 230, 7, 7, INK);
        oval(dc, x + 18, 230, 7, 7, INK);
        rect(dc, x - 9, 212, 19, 16, RGB(180, 125, 74));
        rect(dc, x + 10, 217, 14, 11, BRASS);
        rect(dc, x - 2, 212, 4, 10, CREAM);
        rect(dc, x + 15, 217, 4, 6, CREAM);
    } else if (kind != SCENE_NEW && kind != MOVE && age < 760) {
        int count = kind == FURNITURE || kind == TECH ? s->staff : 1;
        for (int n = 0; n < count; ++n) {
            Spot p = station(s->tier, kind == HIRE ? s->staff - 1 : n);
            if (kind == AMENITIES) p = (Spot){455, 163};
            if (kind == SCENE_WEEK) p = (Spot){264, 143};
            int spread = 3 + (int)age / (kind == SCENE_WEEK ? 28 : 90);
            int size = age < 450 ? 2 : 1;
            for (int i = 0; i < 7; ++i)
                spark(dc, p.x + rays[i].x*spread, p.y + rays[i].y*spread, size, i%2 ? accent : CREAM);
        }
    }
    SIZE ext;
    if (!GetTextExtentPoint32A(dc, label, lstrlenA(label), &ext)) return;
    int x = (SCENE_W - ext.cx)/2;
    int y = 66 - (int)(age < 160 ? (160 - age)/16 : age > 1330 ? (age - 1330)/20 : 0);
    rect(dc, x - 13, y - 4, ext.cx + 26, ext.cy + 8, INK);
    rect(dc, x - 13, y - 4, 4, ext.cy + 8, accent);
    caption(dc, x, y, label, CREAM);
}

void scene_draw(HDC dc, const Sim *s, ULONGLONG tick, bool motion, const SceneCue *cue)
{
    int saved = SaveDC(dc);
    if (!saved) return;
    OffsetWindowOrgEx(dc, -SCENE_X, -SCENE_Y, NULL);
    IntersectClipRect(dc, 0, 0, SCENE_W, SCENE_H);
    SelectObject(dc, GetStockObject(DC_BRUSH));
    SelectObject(dc, GetStockObject(NULL_PEN));
    SetBkMode(dc, TRANSPARENT);
    ULONGLONG idle = motion && !s->closed ? tick : 0;
    ULONGLONG elapsed = motion && cue ? tick - cue->since : SCENE_CUE_MS;
    int mood = sim_forecast(s).mood;
    room(dc, s, idle);
    lounge(dc, s->amenities, idle);
    for (int i = 0; i < offices[s->tier].capacity; ++i) {
        Spot p = station(s->tier, i);
        desk(dc, p, s->furniture, i < s->staff, s->tech != 0);
        technology(dc, p, s->tech, idle, i);
        if (i < s->staff)
            person(dc, p, i, mood, idle, cue && cue->kind == HIRE && i == s->staff - 1 && elapsed < 650
                ? (unsigned)elapsed : 650, s->tech || s->furniture);
    }
    if (elapsed < SCENE_CUE_MS) feedback(dc, s, cue->kind, (unsigned)elapsed);
    RestoreDC(dc, saved);
}
