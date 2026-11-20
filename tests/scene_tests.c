#define WIN32_LEAN_AND_MEAN
#include "scene.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

enum { W = SCENE_W + 2, H = SCENE_H + 2 };
static uint32_t first[W*H], second[W*H];
static uint32_t *pixels;
static HDC dc;

static void render(uint32_t *out, const Sim *s, ULONGLONG tick, bool motion, const SceneCue *cue)
{
    memset(pixels, 0x7f, sizeof first);
    scene_draw(dc, s, tick, motion, cue);
    assert(GdiFlush());
    memcpy(out, pixels, sizeof first);
    for (int x = 0; x < W; ++x)
        assert(out[x] == 0x7f7f7f7f && out[(H - 1)*W + x] == 0x7f7f7f7f);
    for (int y = 0; y < H; ++y)
        assert(out[y*W] == 0x7f7f7f7f && out[y*W + W - 1] == 0x7f7f7f7f);
    assert(out[W + 1] != 0x7f7f7f7f && out[(H - 2)*W + W - 2] != 0x7f7f7f7f);
}

int main(void)
{
    BITMAPINFO info = {0};
    info.bmiHeader.biSize = sizeof info.bmiHeader;
    info.bmiHeader.biWidth = W;
    info.bmiHeader.biHeight = -H;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void *bits = NULL;
    dc = CreateCompatibleDC(NULL);
    assert(dc);
    HBITMAP image = CreateDIBSection(dc, &info, DIB_RGB_COLORS, &bits, NULL, 0);
    assert(image && bits);
    pixels = bits;
    HGDIOBJ old = SelectObject(dc, image);
    assert(old && old != HGDI_ERROR);
    assert(SetViewportOrgEx(dc, 1 - SCENE_X, 1 - SCENE_Y, NULL));

    Sim s = sim_new(1);
    int *levels[] = {&s.furniture, &s.tech, &s.amenities};
    for (int tier = 0; tier < TIER_COUNT; ++tier) {
        s.tier = tier;
        s.staff = offices[tier].capacity - 1;
        for (int i = 0; i < 3; ++i) {
            int limit = i == AMENITIES ? offices[tier].amenity_slots : UPGRADE_LIMIT;
            for (int j = 0; j < limit; ++j) {
                render(first, &s, 900, false, NULL);
                ++*levels[i];
                render(second, &s, 900, false, NULL);
                assert(memcmp(first, second, sizeof first) != 0);
            }
            *levels[i] = 0;
        }
        render(first, &s, 900, false, NULL);
        ++s.staff;
        render(second, &s, 900, false, NULL);
        assert(memcmp(first, second, sizeof first) != 0);
    }

    for (int tier = 1; tier < TIER_COUNT; ++tier) {
        s = sim_new(1);
        s.tier = tier - 1;
        render(first, &s, 0, false, NULL);
        ++s.tier;
        render(second, &s, 0, false, NULL);
        assert(memcmp(first, second, sizeof first) != 0);
    }

    s.staff = offices[s.tier].capacity;
    s.furniture = s.tech = UPGRADE_LIMIT;
    s.amenities = AMENITY_COUNT;
    render(first, &s, 1000, true, NULL);
    render(second, &s, 1320, true, NULL);
    assert(memcmp(first, second, sizeof first) != 0);

    for (int kind = FURNITURE; kind <= SCENE_NEW; ++kind) {
        SceneCue cue = {kind, 1000};
        render(first, &s, 1220, true, NULL);
        render(second, &s, 1220, true, &cue);
        assert(memcmp(first, second, sizeof first) != 0);
        SceneCue earlier = {kind, 760};
        render(first, &s, 1220, true, &earlier);
        if (kind != SCENE_NEW) assert(memcmp(first, second, sizeof first) != 0);
        render(first, &s, 1000 + SCENE_CUE_MS, true, NULL);
        render(second, &s, 1000 + SCENE_CUE_MS, true, &cue);
        assert(memcmp(first, second, sizeof first) == 0);
        render(first, &s, 0, false, NULL);
        render(second, &s, 1220, false, &cue);
        assert(memcmp(first, second, sizeof first) == 0);
    }

    SelectObject(dc, old);
    DeleteObject(image);
    DeleteDC(dc);
    puts("Scene tests passed: office tiers, upgrades, hiring, idle motion, action cues and reduced motion.");
    return 0;
}
