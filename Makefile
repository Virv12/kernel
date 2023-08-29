CFLAGS := -m64 -mgeneral-regs-only -mno-red-zone -Og -mcmodel=kernel -ffunction-sections -fdata-sections -fno-pic -ffreestanding -fno-stack-protector -ggdb3
LDFLAGS := --gc-sections
QEMUFLAGS := -enable-kvm -cpu host -display gtk,zoom-to-fit=on -smp cores=2

OBJS := boot.o main.o allocator.o util.o font.o

run_bios: myos.iso
	qemu-system-x86_64 $(QEMUFLAGS) -cdrom myos.iso

run_uefi: myos.iso
	qemu-system-x86_64 $(QEMUFLAGS) -drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2-ovmf/x64/OVMF_CODE.fd -cdrom myos.iso

mykernel: $(OBJS) link.ld
	ld $(LDFLAGS) -T link.ld -o $@ $(OBJS)

main.o: util.h screen.h allocator.h
allocator.o: allocator.h util.h
util.o: util.h

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

%.o: %.s
	nasm -f elf64 -o $@ $<

font.o: font.bmp
	objcopy -O elf64-x86-64 -B i386 -I binary font.bmp font.o --rename-section .data=.rodata --redefine-sym _binary_font_bmp_start=font_start

myos.iso: mykernel grub.cfg
	mkdir -p iso/boot/grub
	cp mykernel iso/boot/mykernel
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ iso

clean:
	rm -rf *.o mykernel iso myos.iso

.PHONY: run clean
