#pragma once
#include <stdint.h>
#include <stddef.h>

/* Framebuffer information passed from the UEFI bootloader to the kernel.
 * Contains everything needed to draw pixels directly to the screen. */
typedef struct {
    void*        BaseAddress;        /* Physical address of the linear framebuffer */
    size_t       BufferSize;         /* Total size of the framebuffer in bytes */
    unsigned int Width;              /* Horizontal resolution in pixels */
    unsigned int Height;             /* Vertical resolution in pixels */
    unsigned int PixelsPerScanLine;  /* Stride: actual pixels per row (may be > Width) */
} Framebuffer;

/* Boot information structure passed from the bootloader to the kernel.
 * This is the ONLY data the kernel receives from UEFI — everything else
 * (GDT, IDT, drivers) must be set up by the kernel itself. */
typedef struct {
    Framebuffer* framebuffer;
    void*        memoryMap;
    size_t       memoryMapSize;
    size_t       memoryMapDescriptorSize;
    uint32_t     memoryMapDescriptorVersion;
} BootInfo;
