#    Eric Tu and Joshua Eckels
#    CSSE332 - BareMetalOS
#    Team 2-4

all: floppya.img

kernel: kernel.o kernel_asm.o
	ld86 -o kernel -d kernel.o kernel_asm.o

kernel_asm.o: kernel.asm
	as86 kernel.asm -o kernel_asm.o

kernel.o: kernel.c
	bcc -ansi -c -o kernel.o kernel.c

floppya.img: kernel bootload
	dd if=/dev/zero of=floppya.img bs=512 count=2880
	dd if=bootload of=floppya.img bs=512 count=1 conv=notrunc
	dd if=kernel of=floppya.img bs=512 conv=notrunc seek=3

bootload: bootload.asm
	nasm bootload.asm

clean:
	rm -f bootload kernel *.o *~
