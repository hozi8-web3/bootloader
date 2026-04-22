ARCH            = x86_64
BOOTLOADER_OBJS = bootloader/main.o
BOOTLOADER_TARGET = build/BOOTX64.EFI

KERNEL_OBJS     = kernel/main.o kernel/gdt.o kernel/graphics.o kernel/font.o \
                  kernel/string_impl.o kernel/idt.o kernel/keyboard.o kernel/mouse.o
KERNEL_TARGET   = build/kernel.elf

# ---- gnu-efi paths (Ubuntu/Debian/WSL) ----
EFIINC          = /usr/include/efi
EFIINCS         = -I$(EFIINC) -I$(EFIINC)/$(ARCH) -I$(EFIINC)/protocol
EFILIB          = /usr/lib
EFI_CRT_OBJS    = $(EFILIB)/crt0-efi-$(ARCH).o
EFI_LDS         = $(EFILIB)/elf_$(ARCH)_efi.lds

# ---- Tools ----
CC              = gcc
LD              = ld
OBJCOPY         = objcopy

# ---- Bootloader flags ----
CFLAGS          = $(EFIINCS) -fno-stack-protector -fPIC -fshort-wchar \
                  -mno-red-zone -Wall -Wextra -O2 -DEFI_FUNCTION_WRAPPER

LDFLAGS         = -nostdlib -znocombreloc -T $(EFI_LDS) -shared \
                  -Bsymbolic -L $(EFILIB) $(EFI_CRT_OBJS)

# ---- Kernel flags ----
# -mgeneral-regs-only: prevents GCC from using SSE/MMX in kernel code,
# which is critical for __attribute__((interrupt)) handlers to work.
KERNEL_CFLAGS   = -ffreestanding -mno-red-zone -fno-stack-protector \
                  -fno-pic -m64 -O2 -Wall -Wextra -mgeneral-regs-only

KERNEL_LDFLAGS  = -T kernel/linker.ld -nostdlib -z max-page-size=0x1000

# ===========================================================================
# Build targets
# ===========================================================================

.PHONY: all build_dir image clean run

all: build_dir $(BOOTLOADER_TARGET) $(KERNEL_TARGET)

build_dir:
	mkdir -p build

# ---- Bootloader ----
$(BOOTLOADER_TARGET): build/bootloader.so
	$(OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym \
		-j .rel -j .rela -j .rel.* -j .rela.* -j .reloc \
		--target=efi-app-$(ARCH) $^ $@

build/bootloader.so: $(BOOTLOADER_OBJS)
	$(LD) $(LDFLAGS) $(BOOTLOADER_OBJS) -o $@ -lefi -lgnuefi

bootloader/%.o: bootloader/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# ---- Kernel (normal files) ----
kernel/main.o: kernel/main.c
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

kernel/gdt.o: kernel/gdt.c
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

kernel/graphics.o: kernel/graphics.c
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

kernel/font.o: kernel/font.c
	$(CC) $(KERNEL_CFLAGS) -Wno-override-init -c $< -o $@

kernel/string_impl.o: kernel/string_impl.c
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

# ---- Kernel (interrupt handler files) ----
kernel/idt.o: kernel/idt.c
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

kernel/keyboard.o: kernel/keyboard.c
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

kernel/mouse.o: kernel/mouse.c
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

# ---- Link kernel ----
$(KERNEL_TARGET): $(KERNEL_OBJS)
	$(LD) $(KERNEL_LDFLAGS) -o $@ $(KERNEL_OBJS)

# ---- Disk Image ----
image: all
	@echo "=== Creating FAT32 disk image ==="
	dd if=/dev/zero of=build/fat.img bs=1M count=64
	mformat -i build/fat.img -F ::
	mmd -i build/fat.img ::/EFI
	mmd -i build/fat.img ::/EFI/BOOT
	mcopy -i build/fat.img $(BOOTLOADER_TARGET) ::/EFI/BOOT/BOOTX64.EFI
	mcopy -i build/fat.img $(KERNEL_TARGET) ::/kernel.elf
	@echo "=== build/fat.img created successfully ==="

# ---- Run in QEMU ----
run: image
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE.fd \
		-drive if=pflash,format=raw,file=/usr/share/OVMF/OVMF_VARS.fd \
		-drive format=raw,file=build/fat.img \
		-m 256M \
		-net none

clean:
	rm -f bootloader/*.o kernel/*.o build/*.so build/*.EFI build/*.elf build/*.img
