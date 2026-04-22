#include "../include/bootinfo.h"
#include "graphics.h"
#include "gdt.h"
#include "idt.h"
#include "mouse.h"
#include "keyboard.h"
#include "string.h"

/*==========================================================================
 * Orion OS Kernel
 *
 * A minimal but fully functional graphical OS kernel featuring:
 *  - Custom GDT (64-bit segments)
 *  - IDT with PIC remapping and hardware interrupt support
 *  - PS/2 Keyboard driver with full US QWERTY layout + Shift
 *  - PS/2 Mouse driver with cursor tracking
 *  - Double-buffered graphics with modern dark-themed UI
 *  - Multiple interactive windows
 *  - Real-time typed text display
 *=========================================================================*/

/* Fixed backbuffer address — pre-allocated by the bootloader at 64MB */
#define BACKBUFFER_ADDR 0x4000000

/* Application state */
static int current_app = 0;   /* 0=Files, 1=Terminal, 2=About */

/* Simple uptime counter */
static volatile uint64_t tick_count = 0;

/* ---- Draw the "Files" application window ---- */
static void DrawFilesApp(int wx, int wy, int ww, int wh) {
    DrawWindow(wx, wy, ww, wh, "File Manager", 1);

    int cx = wx + 15;
    int cy = wy + 45;

    DrawString("Name", cx, cy, COLOR_TEXT_DIM, COLOR_TRANSPARENT);
    DrawString("Size", cx + 250, cy, COLOR_TEXT_DIM, COLOR_TRANSPARENT);
    DrawString("Type", cx + 340, cy, COLOR_TEXT_DIM, COLOR_TRANSPARENT);
    cy += 15;
    DrawHLine(wx + 10, cy, ww - 20, COLOR_BORDER);
    cy += 8;

    /* File entries */
    const char* files[] = {"kernel.elf", "README.md", "bootloader.efi", "config.sys", "hello.txt"};
    const char* sizes[] = {"128 KB", "4 KB", "64 KB", "1 KB", "2 KB"};
    const char* types[] = {"ELF Binary", "Markdown", "EFI App", "Config", "Text"};
    uint32_t icons[]    = {COLOR_ACCENT3, COLOR_ACCENT2, COLOR_ACCENT, COLOR_ACCENT4, COLOR_TEXT_DIM};

    for (int i = 0; i < 5; i++) {
        /* File icon (colored square) */
        DrawRoundedRect(cx, cy + 2, 10, 10, 2, icons[i]);
        DrawString(files[i], cx + 16, cy + 2, COLOR_TEXT_WHITE, COLOR_TRANSPARENT);
        DrawString(sizes[i], cx + 250, cy + 2, COLOR_TEXT_DIM, COLOR_TRANSPARENT);
        DrawString(types[i], cx + 340, cy + 2, COLOR_TEXT_DIM, COLOR_TRANSPARENT);
        cy += 22;
    }

    /* Storage bar */
    cy += 15;
    DrawString("Storage: 48 MB / 64 MB used", cx, cy, COLOR_TEXT_DIM, COLOR_TRANSPARENT);
    cy += 14;
    DrawProgressBar(cx, cy, ww - 30, 12, 75, COLOR_ACCENT);
}

/* ---- Draw the "Terminal" application window ---- */
static void DrawTerminalApp(int wx, int wy, int ww, int wh) {
    DrawWindow(wx, wy, ww, wh, "Terminal", 1);

    /* Terminal background (darker than window) */
    DrawRect(wx + 5, wy + 31, ww - 10, wh - 36, 0x001E1E2E);

    int cx = wx + 12;
    int cy = wy + 40;

    DrawString("Orion OS v1.0 - Terminal", cx, cy, COLOR_ACCENT2, 0x001E1E2E);
    cy += 14;
    DrawString("Type anything! Keyboard is live.", cx, cy, COLOR_TEXT_DIM, 0x001E1E2E);
    cy += 20;

    /* Show prompt with typed text */
    DrawString("orion@os:~$ ", cx, cy, COLOR_ACCENT, 0x001E1E2E);

    char inputBuf[256];
    GetInputBuffer(inputBuf, 256);
    DrawString(inputBuf, cx + 14 * 8, cy, COLOR_TEXT_WHITE, 0x001E1E2E);

    /* Blinking cursor (simple solid block) */
    int cursorX = cx + (14 + GetInputLength()) * 8;
    tick_count++;
    if ((tick_count / 500) % 2 == 0) {
        DrawRect(cursorX, cy, 8, 10, COLOR_TEXT_WHITE);
    }
}

/* ---- Draw the "About" application window ---- */
static void DrawAboutApp(int wx, int wy, int ww, int wh) {
    DrawWindow(wx, wy, ww, wh, "About This OS", 1);

    int cx = wx + 20;
    int cy = wy + 50;

    /* OS Logo (large H) */
    DrawRoundedRect(cx, cy, 60, 60, 10, COLOR_ACCENT);
    DrawStringScaled("H", cx + 14, cy + 12, 4, COLOR_WINDOW_TITLE, COLOR_TRANSPARENT);

    /* OS info */
    int tx = cx + 80;
    DrawStringScaled("Orion OS", tx, cy, 2, COLOR_TEXT_WHITE, COLOR_TRANSPARENT);
    DrawString("Version 1.0.0 (x86_64)", tx, cy + 22, COLOR_TEXT_DIM, COLOR_TRANSPARENT);
    cy += 50;

    DrawHLine(cx, cy, ww - 40, COLOR_BORDER);
    cy += 12;

    DrawString("Architecture:  x86_64 (64-bit Long Mode)", cx, cy, COLOR_TEXT_WHITE, COLOR_TRANSPARENT); cy += 16;
    DrawString("Bootloader:    UEFI + GNU-EFI", cx, cy, COLOR_TEXT_WHITE, COLOR_TRANSPARENT); cy += 16;
    DrawString("Graphics:      UEFI GOP Framebuffer", cx, cy, COLOR_TEXT_WHITE, COLOR_TRANSPARENT); cy += 16;
    DrawString("Input:         PS/2 Keyboard + Mouse", cx, cy, COLOR_TEXT_WHITE, COLOR_TRANSPARENT); cy += 16;
    DrawString("Memory:        UEFI Memory Map", cx, cy, COLOR_TEXT_WHITE, COLOR_TRANSPARENT); cy += 16;
    DrawString("Interrupts:    PIC + IDT (256 vectors)", cx, cy, COLOR_TEXT_WHITE, COLOR_TRANSPARENT); cy += 24;

    DrawHLine(cx, cy, ww - 40, COLOR_BORDER);
    cy += 12;

    DrawString("Built by Hozaifa", cx, cy, COLOR_ACCENT, COLOR_TRANSPARENT); cy += 14;
    DrawString("Welcome to Orion OS!", cx, cy, COLOR_ACCENT2, COLOR_TRANSPARENT);

    /* System stats */
    cy += 30;
    DrawString("CPU Status: Running", cx, cy, COLOR_ACCENT2, COLOR_TRANSPARENT);
    cy += 16;

    char dimBuf[64];
    char wBuf[16], hBuf[16];
    itoa(GetScreenWidth(), wBuf);
    itoa(GetScreenHeight(), hBuf);
    /* Build "Screen: WxH" manually */
    int pos = 0;
    const char* prefix = "Screen: ";
    while (*prefix) dimBuf[pos++] = *prefix++;
    int k = 0;
    while (wBuf[k]) dimBuf[pos++] = wBuf[k++];
    dimBuf[pos++] = 'x';
    k = 0;
    while (hBuf[k]) dimBuf[pos++] = hBuf[k++];
    dimBuf[pos] = '\0';
    DrawString(dimBuf, cx, cy, COLOR_TEXT_DIM, COLOR_TRANSPARENT);
}

/*==========================================================================
 * Kernel Entry Point
 *=========================================================================*/
void _start(BootInfo* bootInfo) {
    /* Initialize double-buffered graphics */
    InitGraphics(bootInfo->framebuffer, (uint32_t*)BACKBUFFER_ADDR);

    /* Set up our own GDT and IDT (replaces UEFI's) */
    InitGDT();
    InitIDT();

    /* Initialize input devices */
    InitKeyboard();
    InitMouse((int)bootInfo->framebuffer->Width,
              (int)bootInfo->framebuffer->Height);

    int mx, my, lc, rc;
    int scrW = (int)bootInfo->framebuffer->Width;
    int scrH = (int)bootInfo->framebuffer->Height;

    /* Window dimensions and position */
    int ww = scrW * 2 / 3;
    if (ww > 700) ww = 700;
    if (ww < 480) ww = 480;
    int wh = scrH * 2 / 3;
    if (wh > 500) wh = 500;
    if (wh < 300) wh = 300;
    int wx = (scrW - ww) / 2;
    int wy = (scrH - wh - 48) / 2;

    /* ---- Main GUI Loop ---- */
    while (1) {
        /* 1. Draw desktop background */
        DrawDesktop();

        /* 2. Draw the active application window */
        switch (current_app) {
            case 0: DrawFilesApp(wx, wy, ww, wh);    break;
            case 1: DrawTerminalApp(wx, wy, ww, wh);  break;
            case 2: DrawAboutApp(wx, wy, ww, wh);     break;
        }

        /* 3. Draw taskbar (on top of everything except cursor) */
        DrawTaskbar(current_app);

        /* 4. Get mouse state and draw cursor */
        GetMouseState(&mx, &my, &lc, &rc);
        DrawCursor(mx, my);

        /* 5. Handle keyboard input for app switching */
        if (HasKeyPress()) {
            char key = GetLastKey();
            if (key == '1') current_app = 0;
            if (key == '2') current_app = 1;
            if (key == '3') current_app = 2;
        }

        /* 6. Handle mouse clicks on taskbar buttons */
        if (lc) {
            int tbY = scrH - 48;
            if (my >= tbY) {
                /* Check which taskbar button was clicked */
                for (int i = 0; i < 3; i++) {
                    int bx = 60 + i * 70;
                    if (mx >= bx && mx <= bx + 60) {
                        current_app = i;
                    }
                }
            }
        }

        /* 7. Swap to screen */
        SwapBuffers();
    }
}
