#include "mouse.h"
#include "io.h"

/*==========================================================================
 * PS/2 Mouse Driver
 *
 * Handles IRQ12 interrupts. Reads 3-byte packets from the PS/2 auxiliary
 * port and tracks absolute cursor position with proper sign extension
 * and screen clamping.
 *=========================================================================*/

static volatile int mouse_x = 0;
static volatile int mouse_y = 0;
static volatile int mouse_l_click = 0;
static volatile int mouse_r_click = 0;
static int screen_w = 800;
static int screen_h = 600;

static volatile uint8_t mouse_cycle = 0;
static volatile uint8_t mouse_byte[3];

static void MouseWait(uint8_t a_type) {
    uint32_t timeout = 100000;
    if (a_type == 0) {
        while (timeout--) { if ((inb(0x64) & 1) == 1) return; }
    } else {
        while (timeout--) { if ((inb(0x64) & 2) == 0) return; }
    }
}

static void MouseWrite(uint8_t a_write) {
    MouseWait(1);
    outb(0x64, 0xD4);
    MouseWait(1);
    outb(0x60, a_write);
}

static uint8_t MouseRead(void) {
    MouseWait(0);
    return inb(0x60);
}

void InitMouse(int screenWidth, int screenHeight) {
    screen_w = screenWidth;
    screen_h = screenHeight;
    mouse_x  = screenWidth / 2;
    mouse_y  = screenHeight / 2;

    uint8_t status;

    /* Enable auxiliary device (mouse) */
    MouseWait(1);
    outb(0x64, 0xA8);

    /* Enable interrupt for mouse (modify controller config byte) */
    MouseWait(1);
    outb(0x64, 0x20);
    MouseWait(0);
    status = (inb(0x60) | 2);
    MouseWait(1);
    outb(0x64, 0x60);
    MouseWait(1);
    outb(0x60, status);

    /* Tell mouse to use default settings */
    MouseWrite(0xF6);
    MouseRead();

    /* Enable data reporting */
    MouseWrite(0xF4);
    MouseRead();
}

/* IRQ12 handler — called by the IDT */
__attribute__((interrupt))
void MouseHandler(void* frame) {
    (void)frame;

    uint8_t status = inb(0x64);
    if (!(status & 0x20)) {
        /* Not a mouse byte — just send EOI and bail */
        outb(0xA0, 0x20);
        outb(0x20, 0x20);
        return;
    }

    uint8_t byte = inb(0x60);

    if (mouse_cycle == 0) {
        /* First byte must have bit 3 set (per PS/2 protocol) */
        if ((byte & 0x08) == 0) {
            outb(0xA0, 0x20);
            outb(0x20, 0x20);
            return;
        }
    }

    mouse_byte[mouse_cycle++] = byte;

    if (mouse_cycle == 3) {
        mouse_cycle = 0;

        int dX = (int)mouse_byte[1];
        int dY = (int)mouse_byte[2];

        /* Sign extension */
        if (mouse_byte[0] & 0x10) dX |= (int)0xFFFFFF00;
        if (mouse_byte[0] & 0x20) dY |= (int)0xFFFFFF00;

        mouse_x += dX;
        mouse_y -= dY; /* Y axis is inverted in PS/2 */

        /* Button state */
        mouse_l_click = (mouse_byte[0] & 0x01);
        mouse_r_click = (mouse_byte[0] & 0x02) ? 1 : 0;

        /* Clamp to screen bounds */
        if (mouse_x < 0) mouse_x = 0;
        if (mouse_y < 0) mouse_y = 0;
        if (mouse_x >= screen_w) mouse_x = screen_w - 1;
        if (mouse_y >= screen_h) mouse_y = screen_h - 1;
    }

    /* Send End-Of-Interrupt to both PIC2 and PIC1 */
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void GetMouseState(int* x, int* y, int* leftClick, int* rightClick) {
    *x          = mouse_x;
    *y          = mouse_y;
    *leftClick  = mouse_l_click;
    *rightClick = mouse_r_click;
}
