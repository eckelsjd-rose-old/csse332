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

shell: lib_asm.o shell.o
	ld86 -o shell -d shell.o lib_asm.o

lib_asm.o: lib.asm
	as86 lib.asm -o lib_asm.o

shell.o: shell.c
	bcc -ansi -c -o shell.o shell.c

floppya.img: kernel bootload shell
	dd if=/dev/zero of=floppya.img bs=512 count=2880
	dd if=bootload of=floppya.img bs=512 count=1 conv=notrunc
	dd if=kernel of=floppya.img bs=512 conv=notrunc seek=3
	dd if=map.img of=floppya.img bs=512 count=1 seek=1 conv=notrunc
	dd if=dir.img of=floppya.img bs=512 count=1 seek=2 conv=notrunc
	./loadFile message.txt
	./loadFile bigmess.txt
	./loadFile tstpr2
	./loadFile shell

bootload: bootload.asm
	nasm bootload.asm

clean:
	rm -f bootload kernel *.o *~
