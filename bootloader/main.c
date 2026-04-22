#include <efi.h>
#include <efilib.h>
#include "elf.h"
#include "../include/bootinfo.h"

/*==========================================================================
 * UEFI Bootloader for x86_64
 * 
 * This bootloader:
 *  1. Prints "HELLO FROM HOZAIFA" using UEFI Console Output Protocol (Orion OS)
 *  2. Locates and initializes the Graphics Output Protocol (GOP)
 *  3. Opens the EFI System Partition (FAT32) via SimpleFileSystemProtocol
 *  4. Loads kernel.elf, parses its ELF64 headers, loads PT_LOAD segments
 *  5. Retrieves the UEFI memory map
 *  6. Calls ExitBootServices() to hand off hardware control
 *  7. Jumps to the kernel entry point in 64-bit Long Mode
 *=========================================================================*/

/* ---- Utility: memcmp (not available in freestanding UEFI environment) ---- */
int memcmp(const void* aptr, const void* bptr, size_t n) {
    const unsigned char* a = (const unsigned char*)aptr;
    const unsigned char* b = (const unsigned char*)bptr;
    for (size_t i = 0; i < n; i++) {
        if (a[i] < b[i]) return -1;
        else if (a[i] > b[i]) return 1;
    }
    return 0;
}

/* ---- Utility: memset (needed to zero BSS segments) ---- */
void* efi_memset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    for (size_t i = 0; i < n; i++) p[i] = (unsigned char)c;
    return s;
}

/* ---- File I/O: Open a file from the EFI System Partition ---- */
EFI_FILE* OpenFile(EFI_FILE* Directory, CHAR16* Path,
                   EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    EFI_FILE* LoadedFile;

    /* Get the Loaded Image Protocol to find which device we booted from */
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage;
    SystemTable->BootServices->HandleProtocol(
        ImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage);

    /* Get the Simple File System Protocol for that device */
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FileSystem;
    SystemTable->BootServices->HandleProtocol(
        LoadedImage->DeviceHandle,
        &gEfiSimpleFileSystemProtocolGuid, (void**)&FileSystem);

    /* If no directory specified, open the root of the volume */
    if (Directory == NULL) {
        FileSystem->OpenVolume(FileSystem, &Directory);
    }

    EFI_STATUS s = Directory->Open(
        Directory, &LoadedFile, Path, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
    if (EFI_ERROR(s)) {
        return NULL;
    }
    return LoadedFile;
}

/* ---- Graphics: Initialize GOP and return framebuffer info ---- */
Framebuffer framebuffer;

Framebuffer* InitGOP(EFI_SYSTEM_TABLE* SystemTable) {
    EFI_GUID gopGuid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    EFI_STATUS status;

    status = SystemTable->BootServices->LocateProtocol(
        &gopGuid, NULL, (void**)&gop);
    if (EFI_ERROR(status)) {
        Print(L"ERROR: Unable to locate Graphics Output Protocol\n");
        return NULL;
    }

    /* Query current mode; if not started, set mode 0 */
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info;
    UINTN SizeOfInfo;

    status = gop->QueryMode(gop,
        gop->Mode == NULL ? 0 : gop->Mode->Mode, &SizeOfInfo, &info);
    if (EFI_ERROR(status)) {
        gop->SetMode(gop, 0);
    }

    /* Try to find a nice resolution (1024x768 or higher) */
    UINTN bestMode = gop->Mode->Mode;
    UINTN bestWidth = gop->Mode->Info->HorizontalResolution;
    for (UINTN i = 0; i < gop->Mode->MaxMode; i++) {
        gop->QueryMode(gop, i, &SizeOfInfo, &info);
        if (info->HorizontalResolution >= 1024 &&
            info->HorizontalResolution <= 1920 &&
            info->HorizontalResolution > bestWidth) {
            bestMode = i;
            bestWidth = info->HorizontalResolution;
        }
    }
    if (bestMode != gop->Mode->Mode) {
        gop->SetMode(gop, bestMode);
    }

    Print(L"  GOP: %dx%d, Framebuffer at 0x%lx (%ld bytes)\n",
        gop->Mode->Info->HorizontalResolution,
        gop->Mode->Info->VerticalResolution,
        gop->Mode->FrameBufferBase,
        gop->Mode->FrameBufferSize);

    framebuffer.BaseAddress      = (void*)gop->Mode->FrameBufferBase;
    framebuffer.BufferSize       = gop->Mode->FrameBufferSize;
    framebuffer.Width            = gop->Mode->Info->HorizontalResolution;
    framebuffer.Height           = gop->Mode->Info->VerticalResolution;
    framebuffer.PixelsPerScanLine = gop->Mode->Info->PixelsPerScanLine;

    return &framebuffer;
}

/*==========================================================================
 * UEFI Application Entry Point
 *=========================================================================*/
EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);

    /* ---- Step 1: Print welcome message ---- */
    Print(L"============================================\n");
    Print(L"       ORION OS - UEFI Bootloader\n");
    Print(L"       Built by Hozaifa | x86_64\n");
    Print(L"       HELLO FROM HOZAIFA\n");
    Print(L"============================================\n\n");

    /* ---- Step 2: Initialize GOP (Graphics) ---- */
    Print(L"[BOOT] Initializing Graphics Output Protocol...\n");
    Framebuffer* newBuffer = InitGOP(SystemTable);
    if (newBuffer == NULL) {
        Print(L"FATAL: GOP initialization failed\n");
        return EFI_UNSUPPORTED;
    }

    /* ---- Step 3: Load kernel.elf from EFI System Partition ---- */
    Print(L"[BOOT] Loading kernel.elf from EFI System Partition...\n");

    EFI_FILE* KernelFile = OpenFile(NULL, L"kernel.elf", ImageHandle, SystemTable);
    if (KernelFile == NULL) {
        Print(L"FATAL: Could not open kernel.elf\n");
        Print(L"  Make sure kernel.elf is in the root of the ESP.\n");
        Print(L"  Press any key to reboot...\n");
        EFI_INPUT_KEY Key;
        UINTN EventIndex;
        SystemTable->BootServices->WaitForEvent(
            1, &SystemTable->ConIn->WaitForKey, &EventIndex);
        SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &Key);
        return EFI_LOAD_ERROR;
    }

    /* Read ELF header */
    Elf64_Ehdr header;
    UINTN size = sizeof(header);
    KernelFile->Read(KernelFile, &size, &header);

    /* Validate ELF header */
    if (memcmp(&header.e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0 ||
        header.e_ident[EI_CLASS] != ELFCLASS64 ||
        header.e_ident[EI_DATA]  != ELFDATA2LSB ||
        header.e_type            != ET_EXEC ||
        header.e_machine         != EM_X86_64 ||
        header.e_version         != EV_CURRENT)
    {
        Print(L"FATAL: kernel.elf has invalid ELF header\n");
        return EFI_LOAD_ERROR;
    }
    Print(L"  ELF header valid. Entry point: 0x%lx\n", header.e_entry);

    /* ---- Step 4: Load PT_LOAD segments into physical memory ---- */
    Elf64_Phdr* phdrs;
    KernelFile->SetPosition(KernelFile, header.e_phoff);
    UINTN size_phdrs = header.e_phnum * header.e_phentsize;
    SystemTable->BootServices->AllocatePool(EfiLoaderData, size_phdrs, (void**)&phdrs);
    KernelFile->Read(KernelFile, &size_phdrs, phdrs);

    for (int i = 0; i < header.e_phnum; i++) {
        Elf64_Phdr* phdr = (Elf64_Phdr*)((char*)phdrs + i * header.e_phentsize);
        if (phdr->p_type == PT_LOAD) {
            UINTN pages = (phdr->p_memsz + 0xFFF) / 0x1000;
            Elf64_Addr segment = phdr->p_paddr;

            EFI_STATUS allocStatus = SystemTable->BootServices->AllocatePages(
                AllocateAddress, EfiLoaderData, pages, &segment);
            if (EFI_ERROR(allocStatus)) {
                Print(L"FATAL: Cannot allocate %d pages at 0x%lx (error %r)\n",
                    pages, segment, allocStatus);
                return EFI_OUT_OF_RESOURCES;
            }

            /* Zero the entire segment first (handles BSS) */
            efi_memset((void*)segment, 0, phdr->p_memsz);

            /* Read file data into the segment */
            KernelFile->SetPosition(KernelFile, phdr->p_offset);
            UINTN size_segment = phdr->p_filesz;
            KernelFile->Read(KernelFile, &size_segment, (void*)segment);

            Print(L"  Loaded segment: 0x%lx (%ld pages)\n", segment, pages);
        }
    }

    /* Allocate backbuffer for double-buffered graphics (8MB after kernel) */
    UINTN bbPages = (newBuffer->Width * newBuffer->Height * 4 + 0xFFF) / 0x1000;
    EFI_PHYSICAL_ADDRESS bbAddr = 0x4000000; /* 64MB — safely above kernel at 16MB */
    SystemTable->BootServices->AllocatePages(
        AllocateAddress, EfiLoaderData, bbPages, &bbAddr);
    Print(L"  Backbuffer allocated at 0x%lx (%ld pages)\n", bbAddr, bbPages);

    Print(L"[BOOT] Kernel loaded successfully.\n\n");

    /* ---- Step 5: Get Memory Map & Exit Boot Services ---- */
    Print(L"[BOOT] Retrieving memory map and exiting boot services...\n");

    EFI_MEMORY_DESCRIPTOR* Map = NULL;
    UINTN MapSize = 0, MapKey = 0;
    UINTN DescriptorSize = 0;
    UINT32 DescriptorVersion = 0;

    /* First call: get required buffer size */
    SystemTable->BootServices->GetMemoryMap(
        &MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);

    /* Allocate with extra space (allocation itself changes the map) */
    MapSize += 4 * DescriptorSize;
    SystemTable->BootServices->AllocatePool(EfiLoaderData, MapSize, (void**)&Map);

    /* Second call: get the actual map */
    SystemTable->BootServices->GetMemoryMap(
        &MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);

    /* ExitBootServices may fail if the map key changed.
     * The UEFI spec says: if it fails, re-get the map and try again. */
    EFI_STATUS exitStatus = SystemTable->BootServices->ExitBootServices(
        ImageHandle, MapKey);
    if (EFI_ERROR(exitStatus)) {
        /* Map key stale — re-get and retry exactly once */
        SystemTable->BootServices->GetMemoryMap(
            &MapSize, Map, &MapKey, &DescriptorSize, &DescriptorVersion);
        exitStatus = SystemTable->BootServices->ExitBootServices(
            ImageHandle, MapKey);
        if (EFI_ERROR(exitStatus)) {
            /* Cannot recover — system is in undefined state */
            while (1) __asm__ volatile("hlt");
        }
    }

    /* ---- FROM THIS POINT: NO MORE UEFI BOOT SERVICES ---- */
    /* We cannot call Print(), AllocatePool(), or any Boot Service. */

    /* ---- Step 6: Prepare BootInfo and jump to kernel ---- */
    BootInfo bootInfo;
    bootInfo.framebuffer              = newBuffer;
    bootInfo.memoryMap                = Map;
    bootInfo.memoryMapSize            = MapSize;
    bootInfo.memoryMapDescriptorSize  = DescriptorSize;
    bootInfo.memoryMapDescriptorVersion = DescriptorVersion;

    /* Store backbuffer address at a known location the kernel can find.
     * We piggyback this in the area right after BootInfo. The kernel
     * will read it from the fixed address 0x4000000. */

    /* Cast the kernel entry point to a function pointer and call it.
     * The System V ABI (used by GCC on x86_64) passes the first argument
     * in %rdi, which is exactly what our kernel expects. */
    void (*KernelStart)(BootInfo*) =
        ((__attribute__((sysv_abi)) void (*)(BootInfo*))header.e_entry);

    KernelStart(&bootInfo);

    /* Should never reach here */
    while (1) __asm__ volatile("hlt");

    return EFI_SUCCESS;
}
