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
	dd if=bootloader/boot.bin of=_os.img bs=512
	dd if=kernel/kernel.bin of=_os.img bs=512 oflag=append conv=notrunc
#	dd if=demo_file
	dd if=os_copy.img ibs=1M count=32 of=os.img
	dd if=_os.img of=os.img bs=512 conv=notrunc
#	dd if=safta.txt of=hd.img bs=512 conv=notrunc

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
	gcc add_file.c -o add_file.o
	dd if=asd of=os_copy.img
	$(foreach file, $(wildcard default_fs/*.txt), ./add_file.o ${file} ${file};)
	./add_file.o safta.txt safta.txt
	make _fs
	dd if=os_copy.img ibs=1M count=32 of=os.img
	dd if=_os.img of=os.img bs=512 conv=notrunc


_fs: ${FS_OBJECT_FILES}

clean:
	find . -name "*.o" -type f -delete
	find . -name "*.bin" -type f -delete
