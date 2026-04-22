#include "graphics.h"
#include "font.h"
#include "string.h"

/*==========================================================================
 * Double-Buffered Graphics Engine
 *
 * All drawing goes to the backbuffer. SwapBuffers() copies it to the
 * real framebuffer in one shot, eliminating flicker.
 * Uses PixelsPerScanLine (stride) for correct scanline addressing.
 *=========================================================================*/

static Framebuffer* gfb;
static uint32_t*    bb;  /* backbuffer pointer */

void InitGraphics(Framebuffer* fb, uint32_t* backbuf) {
    gfb = fb;
    bb  = backbuf;
}

int GetScreenWidth(void)  { return (int)gfb->Width; }
int GetScreenHeight(void) { return (int)gfb->Height; }

void SwapBuffers(void) {
    /* Copy using PixelsPerScanLine (stride) to handle padding */
    uint32_t* fb_ptr = (uint32_t*)gfb->BaseAddress;
    for (unsigned int y = 0; y < gfb->Height; y++) {
        uint32_t* src = bb + y * gfb->Width;
        uint32_t* dst = fb_ptr + y * gfb->PixelsPerScanLine;
        memcpy(dst, src, gfb->Width * 4);
    }
}

void ClearBackbuffer(uint32_t color) {
    uint32_t total = gfb->Width * gfb->Height;
    for (uint32_t i = 0; i < total; i++) bb[i] = color;
}

void DrawPixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= (int)gfb->Width || y < 0 || y >= (int)gfb->Height) return;
    bb[y * (int)gfb->Width + x] = color;
}

void DrawRect(int x, int y, int w, int h, uint32_t color) {
    for (int j = 0; j < h; j++) {
        for (int i = 0; i < w; i++) {
            DrawPixel(x + i, y + j, color);
        }
    }
}

void DrawRoundedRect(int x, int y, int w, int h, int r, uint32_t color) {
    /* Draw body */
    DrawRect(x + r, y, w - 2*r, h, color);
    DrawRect(x, y + r, r, h - 2*r, color);
    DrawRect(x + w - r, y + r, r, h - 2*r, color);

    /* Draw corners (approximation with filled circle quadrants) */
    for (int cy = 0; cy < r; cy++) {
        for (int cx = 0; cx < r; cx++) {
            if (cx*cx + cy*cy <= r*r) {
                DrawPixel(x + r - cx - 1,     y + r - cy - 1,     color); /* top-left */
                DrawPixel(x + w - r + cx,      y + r - cy - 1,     color); /* top-right */
                DrawPixel(x + r - cx - 1,     y + h - r + cy,      color); /* bottom-left */
                DrawPixel(x + w - r + cx,      y + h - r + cy,      color); /* bottom-right */
            }
        }
    }
}

void DrawBorder(int x, int y, int w, int h, uint32_t color) {
    DrawHLine(x, y, w, color);
    DrawHLine(x, y + h - 1, w, color);
    DrawVLine(x, y, h, color);
    DrawVLine(x + w - 1, y, h, color);
}

void DrawHLine(int x, int y, int w, uint32_t color) {
    for (int i = 0; i < w; i++) DrawPixel(x + i, y, color);
}

void DrawVLine(int x, int y, int h, uint32_t color) {
    for (int i = 0; i < h; i++) DrawPixel(x, y + i, color);
}

void DrawChar(char c, int x, int y, uint32_t fg, uint32_t bg) {
    if (c < 0 || c > 127) return;
    uint8_t* glyph = font8x8_basic[(int)c];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if ((glyph[row] >> (7 - col)) & 1) {
                DrawPixel(x + col, y + row, fg);
            } else if (bg != COLOR_TRANSPARENT) {
                DrawPixel(x + col, y + row, bg);
            }
        }
    }
}

void DrawString(const char* str, int x, int y, uint32_t fg, uint32_t bg) {
    int cx = x;
    while (*str) {
        DrawChar(*str, cx, y, fg, bg);
        cx += 8;
        str++;
    }
}

void DrawStringScaled(const char* str, int x, int y, int scale, uint32_t fg, uint32_t bg) {
    int cx = x;
    while (*str) {
        if (*str >= 0 && *str <= 127) {
            uint8_t* glyph = font8x8_basic[(int)*str];
            for (int row = 0; row < 8; row++) {
                for (int col = 0; col < 8; col++) {
                    uint32_t color = ((glyph[row] >> (7 - col)) & 1) ? fg : bg;
                    if (color == COLOR_TRANSPARENT) continue;
                    DrawRect(cx + col * scale, y + row * scale, scale, scale, color);
                }
            }
        }
        cx += 8 * scale;
        str++;
    }
}

/* ---- Desktop: Draw a smooth gradient background ---- */
void DrawDesktop(void) {
    for (int y = 0; y < (int)gfb->Height; y++) {
        for (int x = 0; x < (int)gfb->Width; x++) {
            /* Dark blue-purple gradient */
            uint8_t r = 25 + (y * 15) / (int)gfb->Height;
            uint8_t g = 25 + (y * 10) / (int)gfb->Height;
            uint8_t b = 40 + (y * 30) / (int)gfb->Height;
            bb[y * (int)gfb->Width + x] = (r << 16) | (g << 8) | b;
        }
    }
}

/* ---- Taskbar: A macOS-style dock at the bottom ---- */
void DrawTaskbar(int selectedApp) {
    int h = 48;
    int y = (int)gfb->Height - h;

    /* Semi-transparent dark bar */
    DrawRect(0, y, (int)gfb->Width, h, COLOR_TASKBAR);

    /* Top border accent line */
    DrawHLine(0, y, (int)gfb->Width, COLOR_BORDER);

    /* Clock area (right side) */
    DrawString("12:00 AM", (int)gfb->Width - 80, y + 18, COLOR_TEXT_WHITE, COLOR_TASKBAR);

    /* "Start" button (left side) */
    DrawRoundedRect(10, y + 8, 32, 32, 6, COLOR_ACCENT);
    DrawStringScaled("H", 18, y + 16, 2, 0x001E1E2E, COLOR_TRANSPARENT);

    /* App indicators */
    const char* apps[] = {"Files", "Term", "About"};
    for (int i = 0; i < 3; i++) {
        int bx = 60 + i * 70;
        uint32_t bg = (i == selectedApp) ? COLOR_TASKBAR_HOVER : COLOR_TASKBAR;
        DrawRoundedRect(bx, y + 8, 60, 32, 6, bg);
        DrawString(apps[i], bx + 10, y + 18, COLOR_TEXT_WHITE, COLOR_TRANSPARENT);

        /* Active indicator dot */
        if (i == selectedApp) {
            DrawRect(bx + 26, y + 42, 8, 3, COLOR_ACCENT);
        }
    }
}

/* ---- Window: Modern dark-themed window with traffic-light buttons ---- */
void DrawWindow(int x, int y, int w, int h, const char* title, int focused) {
    /* Drop shadow */
    DrawRect(x + 4, y + 4, w, h, 0x00080810);

    /* Window body */
    DrawRoundedRect(x, y, w, h, 8, COLOR_WINDOW_BG);

    /* Title bar */
    uint32_t titleColor = focused ? COLOR_WINDOW_TITLE : 0x00252535;
    DrawRoundedRect(x, y, w, 30, 8, titleColor);
    DrawRect(x, y + 22, w, 8, titleColor);  /* Fill gap under rounded top */

    /* Bottom border of title bar */
    DrawHLine(x, y + 30, w, COLOR_BORDER);

    /* Title text */
    DrawString(title, x + 50, y + 10, focused ? COLOR_TEXT_WHITE : COLOR_TEXT_DIM, COLOR_TRANSPARENT);

    /* Traffic light buttons (macOS style) */
    int bsize = 12;
    int by = y + 9;

    /* Close (red) */
    for (int dy = 0; dy < bsize; dy++) {
        for (int dx = 0; dx < bsize; dx++) {
            int cx2 = dx - bsize/2;
            int cy2 = dy - bsize/2;
            if (cx2*cx2 + cy2*cy2 <= (bsize/2)*(bsize/2)) {
                DrawPixel(x + 12 + dx, by + dy, COLOR_CLOSE_BTN);
            }
        }
    }
    /* Minimize (yellow/orange) */
    for (int dy = 0; dy < bsize; dy++) {
        for (int dx = 0; dx < bsize; dx++) {
            int cx2 = dx - bsize/2;
            int cy2 = dy - bsize/2;
            if (cx2*cx2 + cy2*cy2 <= (bsize/2)*(bsize/2)) {
                DrawPixel(x + 28 + dx, by + dy, COLOR_MIN_BTN);
            }
        }
    }
    /* Maximize (green) */
    for (int dy = 0; dy < bsize; dy++) {
        for (int dx = 0; dx < bsize; dx++) {
            int cx2 = dx - bsize/2;
            int cy2 = dy - bsize/2;
            if (cx2*cx2 + cy2*cy2 <= (bsize/2)*(bsize/2)) {
                DrawPixel(x + 44 + dx, by + dy, COLOR_ACCENT2);
            }
        }
    }
}

void DrawButton(int x, int y, int w, int h, const char* label, uint32_t color) {
    DrawRoundedRect(x, y, w, h, 4, color);
    int textX = x + (w - (int)strlen(label) * 8) / 2;
    int textY = y + (h - 8) / 2;
    DrawString(label, textX, textY, COLOR_TEXT_WHITE, COLOR_TRANSPARENT);
}

void DrawProgressBar(int x, int y, int w, int h, int percent, uint32_t color) {
    DrawRoundedRect(x, y, w, h, 4, COLOR_BORDER);
    int fillW = (w - 4) * percent / 100;
    if (fillW > 0) {
        DrawRoundedRect(x + 2, y + 2, fillW, h - 4, 3, color);
    }
}

/* ---- Cursor: Modern arrow pointer ---- */
void DrawCursor(int cx, int cy) {
    /* Arrow cursor shape (12 rows) */
    static const uint8_t cursor[16] = {
        0b10000000,
        0b11000000,
        0b11100000,
        0b11110000,
        0b11111000,
        0b11111100,
        0b11111110,
        0b11111111,
        0b11111100,
        0b11111000,
        0b11011000,
        0b10001100,
        0b00001100,
        0b00000110,
        0b00000110,
        0b00000000,
    };

    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 8; x++) {
            if ((cursor[y] >> (7 - x)) & 1) {
                /* Draw white pixel with a 1px dark border for visibility */
                DrawPixel(cx + x, cy + y, COLOR_CURSOR);
            }
        }
    }
    /* Black outline on right and bottom edges */
    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < 8; x++) {
            if ((cursor[y] >> (7 - x)) & 1) {
                /* Check if the pixel to the right or below is empty */
                int rx = x + 1, ry = y + 1;
                int rightEmpty = (rx >= 8) || !((cursor[y] >> (7 - rx)) & 1);
                int belowEmpty = (ry >= 16) || !((cursor[ry] >> (7 - x)) & 1);
                if (rightEmpty) DrawPixel(cx + x + 1, cy + y, 0x00000000);
                if (belowEmpty) DrawPixel(cx + x, cy + y + 1, 0x00000000);
            }
        }
    }
}
