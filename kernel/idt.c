#include "idt.h"
#include "io.h"
#include "string.h"

/*==========================================================================
 * Interrupt Descriptor Table (IDT)
 *
 * Sets up:
 *  - A default stub handler for all 256 interrupt vectors
 *  - PIC remapping so IRQs 0-15 map to vectors 0x20-0x2F
 *  - Proper gate entries for keyboard (IRQ1) and mouse (IRQ12)
 *=========================================================================*/

struct IDTEntry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  ist;
    uint8_t  type_attr;
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t zero;
} __attribute__((packed));

struct IDTPtr {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

static struct IDTEntry idt[256];
static struct IDTPtr   idt_ptr;

void SetIDTGate(int num, uint64_t base, uint16_t sel, uint8_t flags) {
    idt[num].offset_low  = (base & 0xFFFF);
    idt[num].selector    = sel;
    idt[num].ist         = 0;
    idt[num].type_attr   = flags;
    idt[num].offset_mid  = (base >> 16) & 0xFFFF;
    idt[num].offset_high = (base >> 32) & 0xFFFFFFFF;
    idt[num].zero        = 0;
}

/* ---- Default ISR: just sends EOI and returns ---- */
__attribute__((interrupt))
static void DefaultHandler(void* frame) {
    (void)frame;
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

/* ---- Remap the 8259 PIC ----
 * By default, IRQs 0-7 overlap with CPU exceptions 0-7.
 * We remap them to vectors 0x20-0x2F. */
static void PIC_Remap(void) {
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);

    outb(0x20, 0x11); io_wait();  /* ICW1: begin init, cascade mode */
    outb(0xA0, 0x11); io_wait();

    outb(0x21, 0x20); io_wait();  /* ICW2: PIC1 vector offset = 0x20 */
    outb(0xA1, 0x28); io_wait();  /* ICW2: PIC2 vector offset = 0x28 */

    outb(0x21, 0x04); io_wait();  /* ICW3: PIC1 has slave on IRQ2 */
    outb(0xA1, 0x02); io_wait();  /* ICW3: PIC2 cascade identity */

    outb(0x21, 0x01); io_wait();  /* ICW4: 8086 mode */
    outb(0xA1, 0x01); io_wait();

    /* Restore saved masks */
    outb(0x21, a1);
    outb(0xA1, a2);
}

/* External handlers from keyboard.c and mouse.c */
extern void KeyboardHandler(void* frame);
extern void MouseHandler(void* frame);

void InitIDT(void) {
    idt_ptr.limit = sizeof(idt) - 1;
    idt_ptr.base  = (uint64_t)&idt;

    /* Set all 256 vectors to the default stub handler first */
    for (int i = 0; i < 256; i++) {
        SetIDTGate(i, (uint64_t)DefaultHandler, 0x08, 0x8E);
    }

    PIC_Remap();

    /* IRQ 1 = Keyboard → vector 0x21 */
    SetIDTGate(0x21, (uint64_t)KeyboardHandler, 0x08, 0x8E);

    /* IRQ 12 = PS/2 Mouse → vector 0x2C */
    SetIDTGate(0x2C, (uint64_t)MouseHandler, 0x08, 0x8E);

    /* Unmask IRQ1 (keyboard), IRQ2 (cascade), and IRQ12 (mouse) */
    outb(0x21, 0xF9);  /* PIC1: 1111 1001 = IRQ1 + IRQ2 enabled */
    outb(0xA1, 0xEF);  /* PIC2: 1110 1111 = IRQ12 enabled */

    __asm__ volatile ("lidt %0" : : "m"(idt_ptr));
    __asm__ volatile ("sti");   /* Enable interrupts */
}
