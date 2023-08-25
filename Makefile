CFLAGS := -m64 -mgeneral-regs-only -mno-red-zone -Og -mcmodel=kernel
QEMU_FLAGS := -enable-kvm -cpu host -display gtk,zoom-to-fit=on -smp cores=2

run_iso: myos.iso
	# qemu-system-x86_64 $(QEMU_FLAGS) -cdrom myos.iso
	qemu-system-x86_64 $(QEMU_FLAGS) -drive if=pflash,format=raw,readonly=on,file=/usr/share/edk2-ovmf/x64/OVMF_CODE.fd -cdrom myos.iso

run: mykernel
	qemu-system-x86_64 $(QEMU_FLAGS) -kernel mykernel

mykernel: boot.o main.o allocator.o util.o font.o link.ld
	gcc -m64 -mgeneral-regs-only -mno-red-zone -static -nostdlib -fno-pic -ffreestanding -fno-stack-protector -T link.ld -o $@ boot.o main.o allocator.o util.o font.o -ggdb3

main.o: main.c util.h screen.h allocator.h
	gcc $(CFLAGS) -static -nostdlib -fno-pic -ffreestanding -fno-stack-protector -c -o $@ $< -ggdb3

allocator.o: allocator.c allocator.h util.h
	gcc $(CFLAGS) -static -nostdlib -fno-pic -ffreestanding -fno-stack-protector -c -o $@ $< -ggdb3

util.o: util.c util.h
	gcc $(CFLAGS) -static -nostdlib -fno-pic -ffreestanding -fno-stack-protector -c -o $@ $< -ggdb3

boot.o: boot.s
	nasm -f elf64 -o $@ $<

font.o: font.bmp
	objcopy -O elf64-x86-64 -B i386 -I binary font.bmp font.o --rename-section .data=.rodata --redefine-sym _binary_font_bmp_start=font_start

myos.iso: mykernel grub.cfg
	mkdir -p iso/boot/grub
	cp mykernel iso/boot/mykernel
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ iso

clean:
	rm -rf mykernel *.o iso myos.iso

.PHONY: run clean
