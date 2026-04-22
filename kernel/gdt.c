#include "gdt.h"

/*==========================================================================
 * Global Descriptor Table (GDT)
 *
 * UEFI leaves us with its own GDT. We need to set up our own for a proper
 * OS with well-defined Kernel Code (0x08) and Kernel Data (0x10) segments.
 * In 64-bit Long Mode, segmentation is mostly ignored except for CS.
 *=========================================================================*/

struct GDTDescriptor {
    uint16_t LimitLow;
    uint16_t BaseLow;
    uint8_t  BaseMiddle;
    uint8_t  Access;
    uint8_t  FlagsLimit;
    uint8_t  BaseHigh;
} __attribute__((packed));

struct GDT {
    struct GDTDescriptor Null;       /* 0x00 */
    struct GDTDescriptor KernelCode; /* 0x08 */
    struct GDTDescriptor KernelData; /* 0x10 */
} __attribute__((packed)) __attribute__((aligned(0x10)));

static struct GDT gdt;

struct GDT_PTR {
    uint16_t Limit;
    uint64_t Base;
} __attribute__((packed));

static struct GDT_PTR gdt_ptr;

void InitGDT(void) {
    /* Null descriptor — required by the CPU */
    gdt.Null = (struct GDTDescriptor){0, 0, 0, 0x00, 0x00, 0};

    /* Kernel Code Segment: 64-bit, Present, Ring 0, Execute/Read */
    gdt.KernelCode = (struct GDTDescriptor){0, 0, 0, 0x9A, 0x20, 0};

    /* Kernel Data Segment: Present, Ring 0, Read/Write */
    gdt.KernelData = (struct GDTDescriptor){0, 0, 0, 0x92, 0x00, 0};

    gdt_ptr.Limit = sizeof(gdt) - 1;
    gdt_ptr.Base  = (uint64_t)&gdt;

    /* Load the new GDT and reload all segment registers */
    __asm__ volatile (
        "lgdt %0\n"
        "pushq $0x08\n"
        "lea 1f(%%rip), %%rax\n"
        "pushq %%rax\n"
        "lretq\n"
        "1:\n"
        "mov $0x10, %%ax\n"
        "mov %%ax, %%ds\n"
        "mov %%ax, %%es\n"
        "mov %%ax, %%fs\n"
        "mov %%ax, %%gs\n"
        "mov %%ax, %%ss\n"
        : : "m"(gdt_ptr) : "rax", "memory"
    );
}
