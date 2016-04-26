KERNEL_SOURCE_FILES = $(wildcard kernel/*.c drivers/*.c utils/*.c kernel/*.asm drivers/*.asm utils/*.asm)

_KERNEL_OBJECT_FILES = ${KERNEL_SOURCE_FILES:.c=.o}
KERNEL_OBJECT_FILES = ${_KERNEL_OBJECT_FILES:.asm=.o}

C_LIB_SOURCE_FILES = $(wildcard c/*.c c/*.asm)
_C_LIB_OBJECT_FILES = ${C_LIB_SOURCE_FILES:.c=.o}
C_LIB_OBJECT_FILES = ${_C_LIB_OBJECT_FILES:.asm=.o}

FS_SOURCE_FILES = $(wildcard default_fs/*.c)
FS_OBJECT_FILES = ${FS_SOURCE_FILES:.c=.bin}

all: os.img

os.img: kernel/kernel.bin bootloader/boot.bin
	dd if=/dev/zero of=os.img bs=32M count=1
	dd if=bootloader/boot.bin of=os.img bs=512 conv=notrunc
	dd if=kernel/kernel.bin of=os.img seek=1 bs=512 conv=notrunc
	make fs

kernel/kernel.bin: ${KERNEL_OBJECT_FILES}
	i686-elf-ld -T kernel/kernel.ld $^ -o $@

default_fs/%.bin: ${C_LIB_OBJECT_FILES} default_fs/%.o
	i686-elf-ld -T proc.ld $^ -o $@
	./add_file.o $@

%.o: %.c
	i686-elf-gcc -ffreestanding -masm=intel -c $< -o $@

%.bin: %.asm
	nasm -f bin $< -o $@

%.o: %.asm
	nasm -f elf $< -o $@

fs: 
	rm -rf default_fs/*.bin
	rm -rf default_fs/*.o
	gcc add_file.c -o add_file.o
	dd if=/dev/zero of=os.img bs=1M count=31 seek=1 conv=notrunc
	printf '\x7' | dd of=os.img bs=1 seek=17825792 conv=notrunc 
	$(foreach file, $(wildcard default_fs/*.txt), ./add_file.o ${file} ${file};)
	./add_file.o safta.txt safta.txt
	make _fs


_fs: ${FS_OBJECT_FILES}

clean:
	find . -name "*.o" -type f -delete
	find . -name "*.bin" -type f -delete
