CFLAGS := -m64 -mgeneral-regs-only -mno-red-zone -Wall -Og
QEMU_FLAGS := -enable-kvm -cpu host -display gtk,zoom-to-fit=on -smp cores=2

run_iso: myos.iso
	qemu-system-x86_64 $(QEMU_FLAGS) -cdrom myos.iso

run: mykernel
	qemu-system-x86_64 $(QEMU_FLAGS) -kernel mykernel

mykernel: boot.o main.o link.ld
	gcc -m64 -mgeneral-regs-only -mno-red-zone -static -nostdlib -fno-pic -ffreestanding -fno-stack-protector -T link.ld -o $@ boot.o main.o -ggdb3

main.o: main.c util.h screen.h
	gcc $(CFLAGS) -static -nostdlib -fno-pic -ffreestanding -fno-stack-protector -c -o $@ $< -ggdb3

boot.o: boot.s
	nasm -f elf64 -o $@ $<

myos.iso: mykernel grub.cfg
	mkdir -p iso/boot/grub
	cp mykernel iso/boot/mykernel
	cp grub.cfg iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ iso

clean:
	rm -rf mykernel boot.o main.o iso myos.iso

.PHONY: run clean
