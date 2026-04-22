# Orion OS — UEFI Bootloader + Graphical OS

A fully working UEFI bootloader and graphical operating system for x86_64, built from scratch in C using GNU-EFI.

**Built by Hozaifa** 🚀

## Features

### Bootloader
- UEFI application (PE32+ via `objcopy`)
- Prints "HELLO FROM HOZAIFA" using UEFI Console Output Protocol
- Locates highest resolution via Graphics Output Protocol (GOP)
- Loads `kernel.elf` from FAT32 EFI System Partition
- Parses ELF64 headers and loads `PT_LOAD` segments with BSS zeroing
- Retrieves UEFI memory map with ExitBootServices retry loop
- Jumps to kernel in 64-bit Long Mode via System V ABI

### Kernel / OS
- **GDT**: Custom 64-bit Global Descriptor Table
- **IDT**: 256-vector Interrupt Descriptor Table with PIC remapping
- **PS/2 Keyboard**: Full US QWERTY layout with Shift support, input buffer
- **PS/2 Mouse**: IRQ12 driver with cursor tracking and click detection
- **Graphics Engine**: Double-buffered rendering with:
  - Rounded rectangles, gradients, progress bars
  - 8×8 embedded bitmap font (full ASCII)
  - Scaled text rendering
  - macOS-style window chrome (traffic-light buttons)
  - Bitmap arrow cursor with outline
- **Desktop Environment**:
  - Dark-themed gradient wallpaper
  - Dock-style taskbar with app switching
  - 3 interactive applications:
    - **File Manager** — displays file listing with icons
    - **Terminal** — live keyboard input with blinking cursor
    - **About** — system information display

## Project Structure

```
OrionOS/
├── bootloader/
│   ├── main.c          # UEFI bootloader entry point
│   └── elf.h           # ELF64 header definitions
├── kernel/
│   ├── main.c          # Kernel entry + desktop compositor
│   ├── gdt.c/h         # Global Descriptor Table
│   ├── idt.c/h         # Interrupt Descriptor Table + PIC
│   ├── keyboard.c/h    # PS/2 keyboard driver
│   ├── mouse.c/h       # PS/2 mouse driver
│   ├── graphics.c/h    # Graphics engine
│   ├── font.c/h        # Embedded 8×8 bitmap font
│   ├── string.h        # String utilities (memset, memcpy, itoa)
│   ├── string_impl.c   # Linker symbols for memset/memcpy
│   ├── io.h            # x86 port I/O (inb/outb)
│   └── linker.ld       # Kernel linker script (loads at 16MB)
├── include/
│   └── bootinfo.h      # Shared bootloader↔kernel data structures
├── build/              # Output directory (created by make)
├── .github/workflows/  # CI/CD — auto-builds and releases
├── Makefile            # Complete build system
└── README.md           # This file
```

## Prerequisites (Linux / WSL)

```bash
sudo apt update
sudo apt install -y build-essential gnu-efi mtools qemu-system-x86 ovmf
```

## Build

```bash
make          # Compile bootloader + kernel
make image    # Also create FAT32 disk image (build/fat.img)
```

## Test with QEMU

```bash
make run
```

Or manually:

```bash
qemu-system-x86_64 \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE.fd \
  -drive if=pflash,format=raw,file=/usr/share/OVMF/OVMF_VARS.fd \
  -drive format=raw,file=build/fat.img \
  -m 256M -net none
```

## Test with VirtualBox

1. Download `OrionOS.vdi` from the [Releases](../../releases) page (or convert locally):
   ```bash
   qemu-img convert -f raw -O vdi build/fat.img build/OrionOS.vdi
   ```

2. Create a new VM:
   - **Name**: Orion OS
   - **Type**: Other → Other/Unknown (64-bit)
   - **RAM**: 256 MB+
   - **Hard Disk**: Do not add (we'll attach manually)

3. VM Settings:
   - **System** → Check **Enable EFI (special OSes only)**
   - **Storage** → Add AHCI controller → Attach `OrionOS.vdi`

4. Start the VM and enjoy!

## Controls

| Key/Action | Effect |
|---|---|
| `1` | Switch to File Manager |
| `2` | Switch to Terminal |
| `3` | Switch to About |
| Mouse click on taskbar | Switch applications |
| Any key (in Terminal) | Type text in the terminal prompt |
| Backspace | Delete last character |

## CI/CD

Every push to `main` automatically builds the OS via GitHub Actions. To create a downloadable release:

```bash
git tag v1.0.0
git push origin v1.0.0
```

This creates a GitHub Release with:
- `fat.img` — Raw disk image (QEMU)
- `OrionOS.vdi` — VirtualBox image
- `OrionOS.vmdk` — VMware image

## Architecture

```
UEFI Firmware
  └─→ BOOTX64.EFI (Orion OS Bootloader)
        ├─ InitGOP() → Framebuffer
        ├─ LoadKernel() → ELF → Physical Memory (16MB)
        ├─ GetMemoryMap()
        ├─ ExitBootServices()
        └─→ kernel.elf _start(BootInfo*)
              ├─ InitGDT()
              ├─ InitIDT() + PIC_Remap()
              ├─ InitKeyboard()
              ├─ InitMouse()
              └─ GUI Main Loop
                    ├─ DrawDesktop()
                    ├─ DrawApp() [Files/Terminal/About]
                    ├─ DrawTaskbar()
                    ├─ DrawCursor()
                    └─ SwapBuffers()
```

## License

MIT
