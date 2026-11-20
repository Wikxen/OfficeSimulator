#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include "sim.h"
#include "save.h"
#include "scene.h"

enum { WIDTH = 1000, HEIGHT = 700, FIRST_ACTION = 100, NEXT_WEEK = 110, NEW_GAME };
enum { TINY, BODY, STRONG, VALUE, TITLE, FONT_COUNT };
enum { FRAME_TIMER = 1, FRAME_MS = 40 };

#define INK RGB(23, 48, 47)
#define MUTED RGB(72, 98, 95)
#define PAPER RGB(239, 244, 241)
#define WHITE RGB(255, 255, 255)
#define LINE RGB(214, 226, 221)
#define GREEN RGB(18, 106, 81)
#define MINT RGB(222, 243, 231)
#define RED RGB(154, 57, 48)
#define GOLD RGB(245, 198, 108)

static Sim game;
static HWND actions[ACTION_COUNT], next_button, new_button;
static HFONT fonts[FONT_COUNT];
static int client_w = WIDTH, client_h = HEIGHT;
static char update[180];
static const char *save_status = "";
static SceneCue cue;
static bool has_cue, motion_enabled, motion_error;
static const char *const upgrade_details[3][UPGRADE_LIMIT + 1] = {
    {"Simple desks and chairs", "Walnut desks and storage", "Desk lamps and better seating",
     "Standing desks and ergonomic chairs", "Standing desks and ergonomic chairs installed"},
    {"Laptops for the team", "Dedicated monitors and keyboards", "Dual-monitor workstations",
     "Performance towers", "Performance towers installed"},
    {"Coffee station", "Plants for the break area", "Sofa and coffee table",
     "Arcade cabinet", "Arcade cabinet installed"},
};

static void box(HDC dc, int x, int y, int w, int h, COLORREF color)
{
    RECT r = {x, y, x + w, y + h};
    SetDCBrushColor(dc, color);
    FillRect(dc, &r, (HBRUSH)GetStockObject(DC_BRUSH));
}

static void text(HDC dc, int x, int y, int w, int h, int font, COLORREF color,
    const char *content, UINT align)
{
    RECT r = {x, y, x + w, y + h};
    HGDIOBJ old = SelectObject(dc, fonts[font]);
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextA(dc, content, -1, &r, align | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
    SelectObject(dc, old);
}

static void money(char *out, Money value, bool signed_value)
{
    char digits[24];
    snprintf(digits, sizeof digits, "%lld", value < 0 ? -value : value);
    if (value < 0) *out++ = '-';
    else if (signed_value && value > 0) *out++ = '+';
    *out++ = '$';
    int count = lstrlenA(digits);
    for (int i = 0; i < count; ++i) {
        if (i && (count - i) % 3 == 0) *out++ = ',';
        *out++ = digits[i];
    }
    *out = '\0';
}

static void badge(HDC dc, int x, const char *label, const char *value, COLORREF color)
{
    box(dc, x, 94, 229, 75, WHITE);
    text(dc, x + 14, 102, 200, 21, TINY, MUTED, label, DT_LEFT);
    text(dc, x + 14, 123, 200, 38, VALUE, color, value, DT_LEFT);
}

static void floor_plan(HDC dc)
{
    HGDIOBJ old = SelectObject(dc, fonts[TINY]);
    scene_draw(dc, &game, GetTickCount64(), motion_enabled, has_cue ? &cue : NULL);
    SelectObject(dc, old);
}

static void dashboard(HDC dc)
{
    char buf[128], a[32], b[32];
    Forecast f = sim_forecast(&game);
    const Office *office = &offices[game.tier];
    box(dc, 0, 0, WIDTH, HEIGHT, PAPER);
    box(dc, 0, 0, WIDTH, 80, INK);
    box(dc, 24, 19, 4, 42, GOLD);
    text(dc, 41, 10, 620, 40, TITLE, WHITE, "Office Simulator", DT_LEFT);
    text(dc, 42, 48, 610, 21, BODY, MINT, "Build an office people want to work in.", DT_LEFT);
    snprintf(buf, sizeof buf, "WEEK %02d   /   TIER %d OF %d", game.week, game.tier + 1, TIER_COUNT);
    text(dc, 690, 18, 286, 45, STRONG, MINT, buf, DT_RIGHT);

    money(a, game.cash, false);
    badge(dc, 24, "CASH ON HAND", a, game.cash >= 0 ? INK : RED);
    snprintf(buf, sizeof buf, "%d%%", f.mood);
    badge(dc, 265, "TEAM SATISFACTION", buf, f.mood >= 75 ? GREEN : INK);
    snprintf(buf, sizeof buf, "%d / %d", game.staff, office->capacity);
    badge(dc, 506, "SEATS FILLED", buf, INK);
    money(a, f.net, true);
    badge(dc, 747, "PROJECTED WEEKLY NET", a, f.net >= 0 ? GREEN : RED);

    box(dc, 24, 184, 567, 406, WHITE);
    text(dc, 42, 193, 530, 24, TINY, MUTED, "OFFICE FLOOR", DT_LEFT);
    text(dc, 42, 216, 530, 34, VALUE, INK, office->name, DT_LEFT);
    money(a, office->rent, false);
    snprintf(buf, sizeof buf, "%s / week rent", a);
    text(dc, 360, 221, 213, 28, BODY, MUTED, buf, DT_RIGHT);
    floor_plan(dc);
    text(dc, 42, 520, 200, 25, TINY, MUTED, "TEAM SATISFACTION", DT_LEFT);
    snprintf(buf, sizeof buf, "Reputation: +%d%% revenue", 2*game.reputation);
    text(dc, 281, 520, 292, 25, BODY, MUTED, buf, DT_RIGHT);
    box(dc, 42, 548, 531, 9, LINE);
    box(dc, 42, 548, 531*f.mood/100, 9, f.mood >= 75 ? GREEN : RGB(173, 113, 25));
    money(a, f.revenue, false);
    money(b, f.costs, false);
    snprintf(buf, sizeof buf, "Weekly forecast: %s revenue  |  %s expenses", a, b);
    text(dc, 42, 561, 531, 24, BODY, INK, buf, DT_LEFT);

    box(dc, 607, 184, 369, 406, WHITE);
    text(dc, 625, 192, 335, 24, TINY, MUTED, "INVEST IN YOUR OFFICE", DT_LEFT);
    int levels[] = {game.furniture, game.tech, game.amenities};
    for (int i = FURNITURE; i <= AMENITIES; ++i)
        text(dc, 625, 222 + i*71, 335, 21, BODY, MUTED, upgrade_details[i][levels[i]], DT_LEFT);
    text(dc, 625, 222 + HIRE*71, 335, 21, BODY, MUTED, "Recruit a colleague for $385/week", DT_LEFT);
    text(dc, 625, 222 + MOVE*71, 335, 21, BODY, MUTED, "More room for your team to grow", DT_LEFT);

    box(dc, 24, 604, 952, 71, game.closed ? RGB(250, 230, 226) : MINT);
    if (game.closed) lstrcpyA(buf, "Office closed: expenses exceeded available cash.");
    else if (game.won) lstrcpyA(buf, "Goal reached! Your headquarters is thriving. Keep playing if you like.");
    else snprintf(buf, sizeof buf, "Goal: Full headquarters at %d%%+ satisfaction and a profit (%d/%d weeks).",
        THRIVE_MOOD, game.streak, THRIVE_WEEKS);
    text(dc, 41, 611, 605, 25, STRONG, game.closed ? RED : INK, buf, DT_LEFT);
    text(dc, 41, 637, 605, 26, BODY, MUTED, update, DT_LEFT);
    text(dc, 24, 677, 952, 21, TINY, *save_status || motion_error ? RED : MUTED,
        *save_status ? save_status : motion_error ? "Animation is unavailable. The game is still playable."
        : "Autosaves locally after each action.   |   Tab to select, Space to activate.", DT_LEFT);
}

static void map_dc(HDC dc, int logical_w, int logical_h, int pixel_w, int pixel_h)
{
    SetMapMode(dc, MM_ANISOTROPIC);
    SetWindowExtEx(dc, logical_w, logical_h, NULL);
    SetViewportExtEx(dc, pixel_w, pixel_h, NULL);
}

static RECT floor_pixels(void)
{
    return (RECT){MulDiv(SCENE_X, client_w, WIDTH), MulDiv(SCENE_Y, client_h, HEIGHT),
        MulDiv(SCENE_X + SCENE_W, client_w, WIDTH), MulDiv(SCENE_Y + SCENE_H, client_h, HEIGHT)};
}

static void paint_region(HDC dc, const RECT *dirty)
{
    RECT floor = floor_pixels(), overlap;
    if (IntersectRect(&overlap, &floor, dirty) && EqualRect(&overlap, dirty)) floor_plan(dc);
    else dashboard(dc);
}

static void paint(HWND window)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(window, &ps);
    RECT r = ps.rcPaint;
    int w = r.right - r.left, h = r.bottom - r.top;
    if (!w || !h) {
        EndPaint(window, &ps);
        return;
    }
    HDC buffer = CreateCompatibleDC(dc);
    HBITMAP bitmap = buffer ? CreateCompatibleBitmap(dc, w, h) : NULL;
    if (bitmap) {
        HGDIOBJ old = SelectObject(buffer, bitmap);
        int saved = SaveDC(buffer);
        map_dc(buffer, WIDTH, HEIGHT, client_w, client_h);
        SetViewportOrgEx(buffer, -r.left, -r.top, NULL);
        paint_region(buffer, &r);
        RestoreDC(buffer, saved);
        BitBlt(dc, r.left, r.top, w, h, buffer, 0, 0, SRCCOPY);
        SelectObject(buffer, old);
        DeleteObject(bitmap);
    } else {
        map_dc(dc, WIDTH, HEIGHT, client_w, client_h);
        paint_region(dc, &r);
    }

    if (buffer) DeleteDC(buffer);
    EndPaint(window, &ps);
}

static void update_motion(HWND window)
{
    BOOL enabled = FALSE;
    bool queried = SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &enabled, 0) != FALSE;
    if (queried && enabled && !motion_enabled) {
        motion_enabled = SetTimer(window, FRAME_TIMER, FRAME_MS, NULL) != 0;
        motion_error = !motion_enabled;
    } else if (!queried || !enabled) {
        if (motion_enabled) KillTimer(window, FRAME_TIMER);
        motion_enabled = has_cue = false;
        motion_error = !queried;
    }
    InvalidateRect(window, NULL, FALSE);
}

static void refresh(HWND window)
{
    char label[128], amount[32], description[64];
    for (int i = 0; i < ACTION_COUNT; ++i) {
        Money price = sim_price(&game, i);
        const char *block = sim_block(&game, i);
        if (price) money(amount, price, false);
        if (block && price) snprintf(description, sizeof description, "%s: %s", block, amount);
        else snprintf(description, sizeof description, "%s", block ? block : amount);
        if (i <= AMENITIES) {
            int levels[] = {game.furniture, game.tech, game.amenities};
            snprintf(label, sizeof label, "%s %d/%d\t%s", action_names[i], levels[i], UPGRADE_LIMIT, description);
        } else {
            snprintf(label, sizeof label, "%s\t%s", action_names[i], description);
        }
        SetWindowTextA(actions[i], label);
        EnableWindow(actions[i], !block);
        InvalidateRect(actions[i], NULL, FALSE);
    }
    EnableWindow(next_button, !game.closed);
    InvalidateRect(next_button, NULL, FALSE);
    InvalidateRect(window, NULL, FALSE);
}

static void draw_button(const DRAWITEMSTRUCT *item)
{
    HDC dc = item->hDC;
    int saved = SaveDC(dc);
    int w = item->CtlID < NEXT_WEEK ? 333 : item->CtlID == NEXT_WEEK ? 180 : 102;
    int h = item->CtlID < NEXT_WEEK ? 39 : 40;
    map_dc(dc, w, h, item->rcItem.right, item->rcItem.bottom);
    bool disabled = (item->itemState & ODS_DISABLED) != 0;
    bool pressed = (item->itemState & ODS_SELECTED) != 0;
    bool primary = item->CtlID == NEXT_WEEK && !disabled;
    COLORREF bg = primary ? (pressed ? INK : GREEN) : pressed ? LINE : WHITE;
    COLORREF fg = primary ? WHITE : disabled ? MUTED : INK;
    box(dc, 0, 0, w, h, primary ? bg : LINE);
    box(dc, 1, 1, w - 2, h - 2, bg);
    char label[128];
    GetWindowTextA(item->hwndItem, label, sizeof label);
    char *right = label;
    while (*right && *right != '\t') ++right;
    if (*right) {
        *right++ = '\0';
        text(dc, 10, 0, 165, h, STRONG, fg, label, DT_LEFT);
        text(dc, 175, 0, w - 185, h, TINY, disabled ? MUTED : GREEN, right, DT_RIGHT);
    } else {
        text(dc, 6, 0, w - 12, h, STRONG, fg, label, DT_CENTER);
    }
    if (item->itemState & ODS_FOCUS) {
        RECT r = {3, 3, w - 3, h - 3};
        DrawFocusRect(dc, &r);
    }
    RestoreDC(dc, saved);
}

static HWND button(HWND window, int id, const char *caption, int x, int y, int w, int h)
{
    return CreateWindowExA(0, "BUTTON", caption, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        MulDiv(x, client_w, WIDTH), MulDiv(y, client_h, HEIGHT),
        MulDiv(w, client_w, WIDTH), MulDiv(h, client_h, HEIGHT),
        window, (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
}

static void on_command(HWND window, int id)
{
    int cue_kind;
    if (id >= FIRST_ACTION && id < FIRST_ACTION + ACTION_COUNT) {
        Action action = id - FIRST_ACTION;
        if (!sim_buy(&game, action)) return;
        cue_kind = action;
        if (action == MOVE) snprintf(update, sizeof update, "Moved into %s.", offices[game.tier].name);
        else if (action == HIRE) lstrcpyA(update, "A new colleague joined the team. Check the weekly forecast.");
        else lstrcpyA(update, "Upgrade installed. Check its effect on team satisfaction and profit.");
    } else if (id == NEXT_WEEK) {
        int finished = game.week;
        if (!sim_advance(&game)) {
            if (!game.closed) lstrcpyA(update, "The calendar limit has been reached.");
            refresh(window);
            return;
        }
        char amount[32];
        cue_kind = SCENE_WEEK;
        money(amount, game.last_net, true);
        snprintf(update, sizeof update, "Week %d closed at %s. %s", finished, amount,
            game.closed ? "Start a new game to try again." :
            game.reputation ? "Reputation helps attract more work." : "Aim for 75%+ satisfaction to build reputation.");
    } else if (id == NEW_GAME) {
        if (MessageBoxA(window, "Replace your current office and its autosave?", "Start a new game",
            MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;
        game = sim_new();
        cue_kind = SCENE_NEW;
        lstrcpyA(update, "Start with an upgrade or finish the week to earn more cash.");
    } else return;

    save_status = save_game(&game) ? "" : "Autosave failed. Your progress is only in memory.";
    cue = (SceneCue){cue_kind, GetTickCount64()};
    has_cue = motion_enabled;
    refresh(window);
    HWND focus = GetFocus();
    if (!focus || focus == window || !IsWindowEnabled(focus))
        SetFocus(game.closed ? new_button : next_button);
}

static LRESULT CALLBACK window_proc(HWND window, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE: {
        RECT client;
        GetClientRect(window, &client);
        client_w = client.right;
        client_h = client.bottom;
        const int sizes[] = {13, 15, 16, 29, 34};
        for (int i = 0; i < FONT_COUNT; ++i) {
            fonts[i] = CreateFontA(-sizes[i], 0, 0, 0, i >= STRONG ? FW_SEMIBOLD : FW_NORMAL,
                FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH, "Segoe UI");
            if (!fonts[i]) return -1;
        }
        for (int i = 0; i < ACTION_COUNT; ++i) {
            actions[i] = button(window, FIRST_ACTION + i, "", 625, 246 + 71*i, 333, 39);
            if (!actions[i]) return -1;
        }
        next_button = button(window, NEXT_WEEK, "Finish week", 772, 619, 180, 40);
        new_button = button(window, NEW_GAME, "New game", 658, 619, 102, 40);
        if (!next_button || !new_button) return -1;
        update_motion(window);
        refresh(window);
        return 0;
    }
    case WM_PAINT: paint(window); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_TIMER:
        if (wp == FRAME_TIMER && motion_enabled && !IsIconic(window) && (!game.closed || has_cue)) {
            if (has_cue && GetTickCount64() - cue.since >= SCENE_CUE_MS) has_cue = false;
            RECT floor = floor_pixels();
            InvalidateRect(window, &floor, FALSE);
        }
        return 0;
    case WM_SETTINGCHANGE:
        update_motion(window);
        return 0;
    case WM_COMMAND:
        if (HIWORD(wp) == BN_CLICKED && lp) on_command(window, LOWORD(wp));
        return 0;
    case WM_DRAWITEM:
        if (wp >= FIRST_ACTION && wp <= NEW_GAME) {
            draw_button((const DRAWITEMSTRUCT *)lp);
            return TRUE;
        }
        break;
    case WM_DESTROY:
        if (motion_enabled) KillTimer(window, FRAME_TIMER);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(window, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR cmd, int show)
{
    (void)previous;
    (void)cmd;
    SetProcessDPIAware();
    HANDLE singleton = CreateMutexW(NULL, FALSE, L"Local\\OfficeSimulator.Game");
    if (!singleton || GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBoxA(NULL, singleton ? "The game is already open." : "Could not start the game.",
            "Office Simulator", MB_OK | MB_ICONINFORMATION);
        if (singleton) CloseHandle(singleton);
        return 1;
    }
    game = sim_new();
    switch (load_game(&game)) {
    case SAVE_OK: lstrcpyA(update, "Welcome back. Your office has been restored."); break;
    case SAVE_INVALID:
        save_status = "Autosave was invalid. A fresh office is open.";
        lstrcpyA(update, "Your first action will replace the previous save.");
        break;
    case SAVE_IO_ERROR:
        save_status = "Could not read the autosave. A fresh office is open.";
        lstrcpyA(update, "Your first action will attempt to save this office.");
        break;
    case SAVE_MISSING:
        lstrcpyA(update, "Start with an upgrade or finish the week to earn more cash.");
        break;
    }
    WNDCLASSA wc = {0};
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.lpszClassName = "OfficeSimulator";
    HWND window = NULL;
    if (RegisterClassA(&wc)) {
        DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
        RECT work = {0, 0, 1040, 768}, frame = {0};
        SystemParametersInfoA(SPI_GETWORKAREA, 0, &work, 0);
        AdjustWindowRectEx(&frame, style, FALSE, WS_EX_CONTROLPARENT);
        int border_w = frame.right - frame.left, border_h = frame.bottom - frame.top;
        int sx = (work.right - work.left - border_w - 16)*100 / WIDTH;
        int sy = (work.bottom - work.top - border_h - 16)*100 / HEIGHT;
        int scale = sx < sy ? sx : sy;
        if (scale > 125) scale = 125;
        if (scale > 100 && GetDpiForSystem() <= 96) scale = 100;
        if (scale < 25) scale = 25;
        int w = WIDTH*scale/100 + border_w, h = HEIGHT*scale/100 + border_h;
        window = CreateWindowExA(WS_EX_CONTROLPARENT, wc.lpszClassName, "Office Simulator", style,
            work.left + (work.right - work.left - w)/2, work.top + (work.bottom - work.top - h)/2,
            w, h, NULL, NULL, instance, NULL);
    }
    int code = 1;
    if (window) {
        ShowWindow(window, show);
        if (show != SW_HIDE) SetFocus(game.closed ? new_button : next_button);
        MSG msg;
        BOOL result;
        while ((result = GetMessageA(&msg, NULL, 0, 0)) > 0) {
            if (!IsDialogMessageA(window, &msg)) {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }
        if (result == 0) code = (int)msg.wParam;
        else MessageBoxA(window, "The game could not continue.", "Office Simulator", MB_OK | MB_ICONERROR);
    } else {
        MessageBoxA(NULL, "Could not create the game window.", "Office Simulator", MB_OK | MB_ICONERROR);
    }
    for (int i = 0; i < FONT_COUNT; ++i) if (fonts[i]) DeleteObject(fonts[i]);
    CloseHandle(singleton);
    return code;
}
