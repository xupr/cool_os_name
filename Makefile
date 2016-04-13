KERNEL_SOURCE_FILES = $(wildcard kernel/*.c drivers/*.c utils/*.c kernel/*.asm drivers/*.asm utils/*.asm)

_KERNEL_OBJECT_FILES = ${KERNEL_SOURCE_FILES:.c=.o}
KERNEL_OBJECT_FILES = ${_KERNEL_OBJECT_FILES:.asm=.o}

C_LIB_SOURCE_FILES = $(wildcard c/*.c)
C_LIB_OBJECT_FILES = ${C_LIB_SOURCE_FILES:.c=.o}

all: os.img

os.img: kernel/kernel.bin bootloader/boot.bin
	dd if=bootloader/boot.bin of=_os.img bs=512
	dd if=kernel/kernel.bin of=_os.img bs=512 oflag=append conv=notrunc
#	dd if=demo_file
	dd if=os_copy.img ibs=1M count=32 of=os.img
	dd if=_os.img of=os.img bs=512 conv=notrunc
#	dd if=safta.txt of=hd.img bs=512 conv=notrunc

kernel/kernel.bin: ${KERNEL_OBJECT_FILES}
	i686-elf-ld -T kernel/kernel.ld $^ -o $@

%.o: %.c
	i686-elf-gcc -ffreestanding -masm=intel -c $< -o $@

%.bin: %.asm
	nasm -f bin $< -o $@

%.o: %.asm
	nasm -f elf $< -o $@

shell: ${C_LIB_OBJECT_FILES} proc_entry.o shell.o
	i686-elf-ld -T proc.ld $^ -o shell.bin
	dd if=shell.bin of=os_copy.img conv=notrunc bs=1 count=4096 seek=17858560
	dd if=shell.bin of=os.img conv=notrunc bs=1 count=4096 seek=17858560
	./add_file.o

clean:
	rm */*.o
