#pragma once
#include <stdint.h>
#include "../include/bootinfo.h"

/* ---- Color palette (modern dark theme) ---- */
#define COLOR_BG_DARK      0x001E1E2E   /* Desktop base (dark navy) */
#define COLOR_TASKBAR      0x00181825   /* Taskbar background */
#define COLOR_TASKBAR_HOVER 0x00313244  /* Taskbar button hover */
#define COLOR_WINDOW_BG    0x00302D41   /* Window body */
#define COLOR_WINDOW_TITLE 0x001E1E2E   /* Window title bar */
#define COLOR_ACCENT       0x0089B4FA   /* Blue accent */
#define COLOR_ACCENT2      0x00A6E3A1   /* Green accent */
#define COLOR_ACCENT3      0x00F38BA8   /* Pink/red accent */
#define COLOR_ACCENT4      0x00FAB387   /* Orange accent */
#define COLOR_TEXT_WHITE    0x00CDD6F4   /* Main text */
#define COLOR_TEXT_DIM      0x006C7086   /* Dimmed text */
#define COLOR_BORDER       0x00585B70   /* Borders */
#define COLOR_CLOSE_BTN    0x00F38BA8   /* Close button red */
#define COLOR_MIN_BTN      0x00FAB387   /* Minimize button orange */
#define COLOR_MAX_BTN      0x00A6E3A1   /* Maximize button green */
#define COLOR_CURSOR       0x00FFFFFF   /* Cursor white */
#define COLOR_TRANSPARENT  0xFFFFFFFF   /* Marker: skip bg pixel */

void InitGraphics(Framebuffer* fb, uint32_t* backbuf);
void SwapBuffers(void);
void ClearBackbuffer(uint32_t color);
void DrawPixel(int x, int y, uint32_t color);
void DrawRect(int x, int y, int w, int h, uint32_t color);
void DrawRoundedRect(int x, int y, int w, int h, int r, uint32_t color);
void DrawBorder(int x, int y, int w, int h, uint32_t color);
void DrawHLine(int x, int y, int w, uint32_t color);
void DrawVLine(int x, int y, int h, uint32_t color);
void DrawChar(char c, int x, int y, uint32_t fg, uint32_t bg);
void DrawString(const char* str, int x, int y, uint32_t fg, uint32_t bg);
void DrawStringScaled(const char* str, int x, int y, int scale, uint32_t fg, uint32_t bg);
void DrawDesktop(void);
void DrawTaskbar(int selectedApp);
void DrawWindow(int x, int y, int w, int h, const char* title, int focused);
void DrawButton(int x, int y, int w, int h, const char* label, uint32_t color);
void DrawCursor(int cx, int cy);
void DrawProgressBar(int x, int y, int w, int h, int percent, uint32_t color);
int  GetScreenWidth(void);
int  GetScreenHeight(void);
