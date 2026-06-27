#include "platform_window.h"

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dwmapi.h>

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_BORDER_COLOR
#define DWMWA_BORDER_COLOR 34
#endif

#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif

#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif

extern void *GetWindowHandle(void);

void platform_window_apply_style(bool dark_mode) {
    HWND hwnd = (HWND)GetWindowHandle();
    if (hwnd == NULL) return;

    BOOL use_dark_caption = dark_mode ? TRUE : FALSE;
    DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &use_dark_caption,
                          sizeof(use_dark_caption));

    COLORREF caption_color = dark_mode ? RGB(31, 41, 55) : RGB(245, 247, 250);
    COLORREF text_color = dark_mode ? RGB(243, 244, 246) : RGB(17, 24, 39);
    COLORREF border_color = dark_mode ? RGB(75, 85, 99) : RGB(209, 213, 219);

    DwmSetWindowAttribute(hwnd, DWMWA_CAPTION_COLOR, &caption_color,
                          sizeof(caption_color));
    DwmSetWindowAttribute(hwnd, DWMWA_TEXT_COLOR, &text_color, sizeof(text_color));
    DwmSetWindowAttribute(hwnd, DWMWA_BORDER_COLOR, &border_color,
                          sizeof(border_color));
}

#else

void platform_window_apply_style(bool dark_mode) {
    (void)dark_mode;
}

#endif
