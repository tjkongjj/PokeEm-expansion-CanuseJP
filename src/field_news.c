#include "global.h"
#include "field_news.h"
#include "bg.h"
#include "gpu_regs.h"
#include "main.h"
#include "malloc.h"
#include "map_name_popup.h"
#include "menu.h"
#include "overworld.h"
#include "palette.h"
#include "rtc.h"
#include "script.h"
#include "sound.h"
#include "string_util.h"
#include "task.h"
#include "text.h"
#include "window.h"
#include "constants/characters.h"
#include "constants/songs.h"
#include "config/overworld.h"

// Field news ticker (時報テロップ), adapted from a CFRU-JP add-on
// (field_news.c) to this decomp codebase.
// At every hour divisible by 3, a band slides in at the top of the
// screen and a one-line news text scrolls from right to left once,
// with a SE_DING_DONG chime. 0:00 and 18:00 have dedicated texts.
// The ticker never shows during battles, menus, scripts, palette
// fades or while the map name popup is up; if it is interrupted it
// restarts from the beginning once the field is free again.

#if OW_FIELD_NEWS_TICKER

#define NEWS_BASE_TILE    0x240 // BG0 tiles 0x240-0x27F are unused in the field
#define NEWS_PAL          15
#define NEWS_FONT         FONT_NORMAL
#define NEWS_SPEED        1 // pixels per frame
#define NEWS_SLIDE_FRAMES 8
#define NEWS_SLIDE_Y      16
#define NEWS_START_GAP    16
#define NEWS_CHUNK_PX     224
#define NEWS_TEXT_MAX     160
#define NEWS_SE           SE_DING_DONG

enum { NEWS_OFF, NEWS_SLIDE_IN, NEWS_SCROLL, NEWS_SLIDE_OUT };

struct FieldNewsData
{
    u8 *bitmap;
    u16 columns;
    u16 scroll;
    s16 ringCol[32];
    u8 ring[64 * 32];
    u16 savedHofs;
    u16 savedVofs;
};

static u8 sLastHour;
static bool8 sHourKnown = FALSE;
static bool8 sPending = FALSE;
static u8 sPendingHour = 0;
static u8 sState = NEWS_OFF;
static u16 sTimer = 0;
static u32 sLastFrame = 0;
static struct FieldNewsData *sData = NULL;

static const u8 sText_NewsHour[] = _("じの ポケモンニュース！ ");
static const u8 sText_NewsNow[] = _("ただいま ");
static const u8 sText_NewsPeriodEnd[] = _("の じかんです");
static const u8 sText_NewsMidnight[] = _("ポケモンの ぶんぷが かわった きが する");
static const u8 sText_NewsEveningSoon[] = _("もうすぐ ゆうがた です");
static const u8 sText_PeriodMorning[] = _("あさ");
static const u8 sText_PeriodDay[] = _("ひる");
static const u8 sText_PeriodEvening[] = _("ゆうがた");
static const u8 sText_PeriodNight[] = _("よる");
static const u8 *const sText_PeriodNames[TIMES_OF_DAY_COUNT] =
{
    [TIME_MORNING] = sText_PeriodMorning,
    [TIME_DAY]     = sText_PeriodDay,
    [TIME_EVENING] = sText_PeriodEvening,
    [TIME_NIGHT]   = sText_PeriodNight,
};

bool8 IsFieldNewsActive(void)
{
    return sState != NEWS_OFF;
}

/* Builds the text for the given (pending) hour. Always succeeds. */
static bool8 FieldNews_BuildText(u8 *dest, u8 hour)
{
    u8 *p = dest;

    if (hour == 0)
    {
        // Midnight: the daily wild encounter groups have rotated
        StringCopy(p, sText_NewsMidnight);
        return TRUE;
    }
    if (hour == 18)
    {
        // Evening (19:00-20:00) never lands on a multiple of 3,
        // so 18:00 announces it in advance instead
        StringCopy(p, sText_NewsEveningSoon);
        return TRUE;
    }
    p = ConvertIntToDecimalStringN(p, hour, STR_CONV_MODE_LEFT_ALIGN, hour >= 10 ? 2 : 1);
    p = StringCopy(p, sText_NewsHour);
    p = StringCopy(p, sText_NewsNow);
    p = StringCopy(p, sText_PeriodNames[GetTimeOfDay()]);
    p = StringCopy(p, sText_NewsPeriodEnd);
    *p = EOS;
    return TRUE;
}

static u32 FieldNews_CharLength(const u8 *str)
{
    // The news texts are plain display text: a kanji is a 3-byte
    // EXT_CTRL_CODE sequence, every other character is one byte.
    // (No placeholders or control codes are used in these texts.)
    return str[0] == EXT_CTRL_CODE_BEGIN ? 3 : 1;
}

/* Renders the whole text into a heap bitmap of tile columns. */
static u8 *FieldNews_Render(const u8 *text, u16 *columns)
{
    static const struct WindowTemplate sRenderWindow = {
        .bg = 0,
        .tilemapLeft = 0,
        .tilemapTop = 0,
        .width = 30,
        .height = 2,
        .paletteNum = NEWS_PAL,
        .baseBlock = NEWS_BASE_TILE
    };
    u8 chunk[NEWS_TEXT_MAX + 1];
    u32 total = GetStringWidth(NEWS_FONT, text, 0), offset = 0, i, n;
    u8 *bitmap;
    u8 win;

    *columns = (total + 7) / 8 + 1;
    bitmap = AllocZeroed(*columns * 64);
    if (bitmap == NULL)
        return NULL;
    win = AddWindow(&sRenderWindow);
    if (win == WINDOW_NONE)
    {
        Free(bitmap);
        return NULL;
    }

    while (*text != EOS)
    {
        const u8 *tiles;
        u32 w, tx, ty, b, colOff = offset / 8, sub = offset % 8;
        u32 sum = 0;

        // Take as many whole characters as fit into one chunk
        n = 0;
        while (text[n] != EOS)
        {
            u8 one[4];
            u32 len = FieldNews_CharLength(&text[n]);
            u32 cw, j;

            if (n + len > NEWS_TEXT_MAX)
                break;
            for (j = 0; j < len; j++)
                one[j] = text[n + j];
            one[len] = EOS;
            cw = GetStringWidth(NEWS_FONT, one, 0);
            if (n > 0 && sum + cw > NEWS_CHUNK_PX)
                break;
            sum += cw;
            for (j = 0; j < len; j++)
                chunk[n + j] = text[n + j];
            n += len;
        }
        chunk[n] = EOS;
        w = GetStringWidth(NEWS_FONT, chunk, 0);
        FillWindowPixelBuffer(win, PIXEL_FILL(0));
        AddTextPrinterParameterized(win, NEWS_FONT, chunk, sub, 0, TEXT_SKIP_DRAW, NULL);
        tiles = (const u8 *)GetWindowAttribute(win, WINDOW_TILE_DATA);

        for (tx = 0; tx < 30 && tx * 8 < sub + w + 8; ++tx)
        {
            if (colOff + tx >= *columns)
                break;
            for (ty = 0; ty < 2; ++ty)
            {
                const u8 *src = &tiles[(ty * 30 + tx) * 32];
                u8 *dst = &bitmap[(colOff + tx) * 64 + ty * 32];
                for (b = 0; b < 32; ++b)
                {
                    u8 s = src[b];
                    if (s & 0x0F) dst[b] = (dst[b] & 0xF0) | (s & 0x0F);
                    if (s & 0xF0) dst[b] = (dst[b] & 0x0F) | (s & 0xF0);
                }
            }
        }
        offset += w;
        text += n;
        if (n == 0)
            break;
    }
    RemoveWindow(win);

    // Empty pixels become the band's background color (palette index 1)
    for (i = 0; i < *columns * 64u; ++i)
    {
        u8 v = bitmap[i];
        if ((v & 0x0F) == 0) v |= 0x01;
        if ((v & 0xF0) == 0) v |= 0x10;
        bitmap[i] = v;
    }
    return bitmap;
}

static void FieldNews_LoadColumn(struct FieldNewsData *d, u32 c, s32 vcol)
{
    s32 textCol = vcol - (240 + NEWS_START_GAP) / 8;
    u8 *top = &d->ring[c * 32];
    u8 *bottom = &d->ring[(32 + c) * 32];
    u32 b;

    if (textCol >= 0 && textCol < d->columns)
    {
        for (b = 0; b < 32; ++b)
        {
            top[b] = d->bitmap[textCol * 64 + b];
            bottom[b] = d->bitmap[textCol * 64 + 32 + b];
        }
    }
    else
    {
        for (b = 0; b < 32; ++b)
            top[b] = bottom[b] = 0x11;
    }
    d->ringCol[c] = vcol;
    LoadBgTiles(0, top, 32, NEWS_BASE_TILE + c);
    LoadBgTiles(0, bottom, 32, NEWS_BASE_TILE + 32 + c);
}

static void FieldNews_UpdateRing(struct FieldNewsData *d)
{
    s32 first = d->scroll / 8, v;
    for (v = first; v < first + 32; ++v)
    {
        u32 c = v % 32;
        if (d->ringCol[c] != v)
            FieldNews_LoadColumn(d, c, v);
    }
}

static void FieldNews_SetTilemap(bool8 show)
{
    u32 c;
    if (show)
    {
        for (c = 0; c < 32; ++c)
        {
            FillBgTilemapBufferRect(0, NEWS_BASE_TILE + c, c, 0, 1, 1, NEWS_PAL);
            FillBgTilemapBufferRect(0, NEWS_BASE_TILE + 32 + c, c, 1, 1, 1, NEWS_PAL);
        }
    }
    else
        FillBgTilemapBufferRect(0, 0, 0, 0, 32, 2, 0);
    CopyBgTilemapBufferToVram(0);
}

static bool8 FieldNews_ShouldHide(void)
{
    return gMain.callback2 != CB2_Overworld
        || ScriptContext_IsEnabled()
        || ArePlayerFieldControlsLocked()
        || gPaletteFade.active
        || IsMapNamePopupActive();
}

/* Whether the band's BG0 tilemap rows may be cleared when stopping.
   The start menu and the map name popup draw their own windows
   over the top rows, so while either of them owns the screen the
   tilemap is left alone; the restarted ticker repaints the rows
   when it begins again. During scripts (dialogue) nothing else
   uses the top rows, so clearing is safe there.
   The BG offsets are separate: they shift the whole BG0 layer,
   so they are always restored on stop (see FieldNews_Stop). */
static bool8 FieldNews_CanRestoreBg(void)
{
    if (IsMapNamePopupActive())
        return FALSE;
    if (ArePlayerFieldControlsLocked() && !ScriptContext_IsEnabled())
        return FALSE;
    return TRUE;
}

static void FieldNews_Stop(bool8 finished, bool8 restoreBg)
{
    struct FieldNewsData *d = sData;
    if (d != NULL)
    {
        if (restoreBg)
        {
            // Always restore the offsets: every other BG0 user
            // (start menu windows, dialogue, ...) assumes them at
            // their normal values, and a leftover scroll offset
            // shifts the whole layer
            SetGpuReg(REG_OFFSET_BG0HOFS, d->savedHofs);
            SetGpuReg(REG_OFFSET_BG0VOFS, d->savedVofs);
            if (FieldNews_CanRestoreBg())
                FieldNews_SetTilemap(FALSE);
        }
        if (d->bitmap != NULL)
            Free(d->bitmap);
        Free(d);
        sData = NULL;
    }
    sState = NEWS_OFF;
    if (!finished)
        sPending = TRUE;
}

void FieldNews_Abort(void)
{
    if (IsFieldNewsActive())
        FieldNews_Stop(FALSE, TRUE);
}

static void FieldNews_Update(void)
{
    struct FieldNewsData *d = sData;

    switch (sState)
    {
    case NEWS_SLIDE_IN:
        SetGpuReg(REG_OFFSET_BG0VOFS, d->savedVofs + NEWS_SLIDE_Y - NEWS_SLIDE_Y * sTimer / NEWS_SLIDE_FRAMES);
        if (++sTimer > NEWS_SLIDE_FRAMES)
        {
            SetGpuReg(REG_OFFSET_BG0VOFS, d->savedVofs);
            sState = NEWS_SCROLL;
            sTimer = 0;
        }
        break;
    case NEWS_SCROLL:
        d->scroll += NEWS_SPEED;
        FieldNews_UpdateRing(d);
        SetGpuReg(REG_OFFSET_BG0HOFS, d->scroll & 0xFF);
        if (d->scroll >= 240 + NEWS_START_GAP + d->columns * 8)
        {
            sState = NEWS_SLIDE_OUT;
            sTimer = 0;
        }
        break;
    case NEWS_SLIDE_OUT:
        SetGpuReg(REG_OFFSET_BG0VOFS, d->savedVofs + NEWS_SLIDE_Y * sTimer / NEWS_SLIDE_FRAMES);
        if (++sTimer > NEWS_SLIDE_FRAMES)
            FieldNews_Stop(TRUE, TRUE);
        break;
    }
}

static bool8 FieldNews_Start(u8 hour)
{
    u8 text[NEWS_TEXT_MAX + 1];
    struct FieldNewsData *d;
    u32 c;

    if (!FieldNews_BuildText(text, hour))
        return TRUE;
    d = AllocZeroed(sizeof(struct FieldNewsData));
    if (d == NULL)
        return FALSE;
    d->bitmap = FieldNews_Render(text, &d->columns);
    if (d->bitmap == NULL)
    {
        Free(d);
        return FALSE;
    }
    d->savedHofs = GetGpuReg(REG_OFFSET_BG0HOFS);
    d->savedVofs = GetGpuReg(REG_OFFSET_BG0VOFS);
    d->scroll = 0;
    for (c = 0; c < 32; ++c)
        d->ringCol[c] = -1;
    // Make sure palette 15 holds the standard window palette
    LoadMessageBoxAndBorderGfx();
    FieldNews_UpdateRing(d);
    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, d->savedVofs + NEWS_SLIDE_Y);
    FieldNews_SetTilemap(TRUE);

    sData = d;
    sState = NEWS_SLIDE_IN;
    sTimer = 0;
    sLastFrame = gMain.vblankCounter2;
    PlaySE(NEWS_SE);
    return TRUE;
}

void Task_FieldNews(u8 taskId)
{
    u8 hour;
    u32 frame = gMain.vblankCounter2;

    RtcCalcLocalTime();
    hour = gLocalTime.hours;

    if (!sHourKnown)
    {
        // First tick after entering the field: never fire for the
        // hour the game was started in, only from the next one
        sHourKnown = TRUE;
        sLastHour = hour;
        sLastFrame = frame;
        return;
    }
    if (hour != sLastHour)
    {
        sLastHour = hour;
        if (hour % 3 == 0)
        {
            sPending = TRUE;
            sPendingHour = hour;
        }
    }

    if (sState != NEWS_OFF)
    {
        if ((u32)(frame - sLastFrame) > 4)
            // The task did not run for a while (battle, menu, ...):
            // drop the half-shown ticker without touching the BG,
            // it restarts from the beginning via the pending flag
            FieldNews_Stop(FALSE, FALSE);
        else if (FieldNews_ShouldHide())
            FieldNews_Stop(FALSE, TRUE);
        else
            FieldNews_Update();
    }
    sLastFrame = frame;

    if (!sPending || sState != NEWS_OFF || FieldNews_ShouldHide())
        return;
    if (FieldNews_Start(sPendingHour))
        sPending = FALSE;
    sLastFrame = gMain.vblankCounter2;
}

#else

void Task_FieldNews(u8 taskId)
{
}

bool8 IsFieldNewsActive(void)
{
    return FALSE;
}

void FieldNews_Abort(void)
{
}

#endif
