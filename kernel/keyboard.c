#include "keyboard.h"
#include "io.h"

/*==========================================================================
 * PS/2 Keyboard Driver
 *
 * Handles IRQ1 interrupts. Reads scan codes from port 0x60 and translates
 * them to ASCII using a US QWERTY scancode lookup table.
 * Maintains a circular input buffer for the OS to consume.
 *=========================================================================*/

/* US QWERTY Scancode Set 1 → ASCII lookup table */
static const char scancode_to_ascii[128] = {
    0,   27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,   'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,   '\\','z','x','c','v','b','n','m',',','.','/', 0,
    '*', 0,  ' ', 0, 0,0,0,0,0,0,0,0,0,0, 0, 0,
    0,0,0,'-', 0,0,0,'+', 0,0,0,0,0, 0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0
};

/* Shift scancode → ASCII lookup */
static const char scancode_to_ascii_shift[128] = {
    0,   27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,   'A','S','D','F','G','H','J','K','L',':','"','~',
    0,   '|','Z','X','C','V','B','N','M','<','>','?', 0,
    '*', 0,  ' ', 0, 0,0,0,0,0,0,0,0,0,0, 0, 0,
    0,0,0,'-', 0,0,0,'+', 0,0,0,0,0, 0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0
};

#define INPUT_BUF_SIZE 256

static volatile char  input_buffer[INPUT_BUF_SIZE];
static volatile int   input_len = 0;
static volatile char  last_key  = 0;
static volatile int   has_key   = 0;
static volatile int   shift_held = 0;

void InitKeyboard(void) {
    input_len = 0;
    last_key  = 0;
    has_key   = 0;
}

/* Called from IDT as the IRQ1 handler */
__attribute__((interrupt))
void KeyboardHandler(void* frame) {
    (void)frame;

    uint8_t scancode = inb(0x60);

    /* Track Shift key state */
    if (scancode == 0x2A || scancode == 0x36) {
        shift_held = 1;
        outb(0x20, 0x20);
        return;
    }
    if (scancode == 0xAA || scancode == 0xB6) {
        shift_held = 0;
        outb(0x20, 0x20);
        return;
    }

    /* Only handle key-down events (bit 7 = 0) */
    if (scancode < 128) {
        char c;
        if (shift_held) {
            c = scancode_to_ascii_shift[scancode];
        } else {
            c = scancode_to_ascii[scancode];
        }

        if (c != 0) {
            last_key = c;
            has_key  = 1;

            if (c == '\b') {
                /* Backspace: remove last character */
                if (input_len > 0) input_len--;
            } else if (c != '\n' && c != '\t') {
                /* Append to input buffer */
                if (input_len < INPUT_BUF_SIZE - 1) {
                    input_buffer[input_len++] = c;
                    input_buffer[input_len]   = '\0';
                }
            }
        }
    }

    /* Send End-Of-Interrupt to PIC1 */
    outb(0x20, 0x20);
}

char GetLastKey(void) {
    char k = last_key;
    last_key = 0;
    has_key  = 0;
    return k;
}

int HasKeyPress(void) {
    return has_key;
}

void GetInputBuffer(char* buf, int maxLen) {
    int len = input_len;
    if (len > maxLen - 1) len = maxLen - 1;
    for (int i = 0; i < len; i++) {
        buf[i] = input_buffer[i];
    }
    buf[len] = '\0';
}

int GetInputLength(void) {
    return input_len;
}
