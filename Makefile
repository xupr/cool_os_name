all: bootloader/boot.o kernel/kernel.o
	dd if=bootloader/boot.o of=_os.img bs=512
	dd if=kernel/kernel.o of=_os.img bs=512 oflag=append conv=notrunc
#	dd if=demo_file
	dd if=os_copy.img ibs=1M count=32 of=os.img
	dd if=_os.img of=os.img bs=512 conv=notrunc
#	dd if=safta.txt of=hd.img bs=512 conv=notrunc

shell.o:
	i686-elf-gcc -ffreestanding -masm=intel -c -o shell.o shell.c

c/stdio.o:
	i686-elf-gcc -ffreestanding -masm=intel -c -o c/stdio.o c/stdio.c

c/stdlib.o:
	i686-elf-gcc -ffreestanding -masm=intel -c -o c/stdlib.o c/stdlib.c

bootloader/boot.o: 
	nasm -f bin -o bootloader/boot.o bootloader/boot.asm

kernel/kernel.o: utils/portio.o drivers/screen.o kernel/interrupts.o kernel/filesystem.o kernel/memory.o kernel/process.o kernel/system_call.o kernel/system_call_entry.o drivers/keyboard_entry.o drivers/keyboard.o drivers/ata.o drivers/ata_entry.o utils/heap.o utils/list.o utils/string.o utils/stdlib.o
	i686-elf-gcc -ffreestanding -masm=intel -c -o kernel/_kernel.o kernel/kernel.c
	nasm -f elf kernel/kernel_loader.asm
	i686-elf-ld -T kernel/kernel.ld -o kernel/kernel.o kernel/kernel_loader.o kernel/_kernel.o kernel/filesystem.o kernel/memory.o kernel/process.o kernel/system_call.o kernel/system_call_entry.o utils/portio.o drivers/screen.o kernel/interrupts.o drivers/keyboard_entry.o drivers/keyboard.o drivers/ata.o drivers/ata_entry.o utils/heap.o utils/list.o utils/string.o utils/stdlib.o

kernel/filesystem.o:
	i686-elf-gcc -ffreestanding -masm=intel -c -o kernel/filesystem.o kernel/filesystem.c

kernel/memory.o:
	i686-elf-gcc -ffreestanding -masm=intel -c -o kernel/memory.o kernel/memory.c

kernel/interrupts.o: 	
	i686-elf-gcc -ffreestanding -masm=intel -c -o kernel/interrupts.o kernel/interrupts.c

kernel/process.o:
	i686-elf-gcc -ffreestanding -masm=intel -c -o kernel/process.o kernel/process.c

kernel/system_call.o:
	i686-elf-gcc -ffreestanding -masm=intel -c -o kernel/system_call.o kernel/system_call.c

kernel/system_call_entry.o:
	nasm -f elf -o kernel/system_call_entry.o kernel/system_call_entry.asm

drivers/keyboard_entry.o:
	nasm -f elf -o drivers/keyboard_entry.o drivers/keyboard_entry.asm

drivers/keyboard.o:
	i686-elf-gcc -ffreestanding -masm=intel -c -o drivers/keyboard.o drivers/keyboard.c

drivers/screen.o: 
	i686-elf-gcc -ffreestanding -masm=intel -c -o drivers/screen.o drivers/screen.c

drivers/ata.o:
	i686-elf-gcc -ffreestanding -masm=intel -c -o drivers/ata.o drivers/ata.c

drivers/ata_entry.o:
	nasm -f elf -o drivers/ata_entry.o drivers/ata_entry.asm

utils/portio.o: 
	i686-elf-gcc -ffreestanding -masm=intel -c -o utils/portio.o utils/portio.c

utils/heap.o:
	i686-elf-gcc -ffreestanding -masm=intel -c -o utils/heap.o utils/heap.c

utils/list.o:
	i686-elf-gcc -ffreestanding -masm=intel -c -o utils/list.o utils/list.c

utils/string.o:
	i686-elf-gcc -ffreestanding -masm=intel -c -o utils/string.o utils/string.c

utils/stdlib.o:
	i686-elf-gcc -ffreestanding -masm=intel -c -o utils/stdlib.o utils/stdlib.c

shell:
	i686-elf-gcc -ffreestanding -masm=intel -c -o shell.o shell.c
	i686-elf-gcc -ffreestanding -masm=intel -c -o c/stdlib.o c/stdlib.c
	i686-elf-gcc -ffreestanding -masm=intel -c -o c/stdio.o c/stdio.c
	i686-elf-gcc -ffreestanding -masm=intel -c -o c/string.o c/string.c
	nasm -f elf proc_entry.asm -o proc_entry.o
	i686-elf-ld proc_entry.o shell.o c/stdio.o c/string.o c/stdlib.o -o shell.bin -T proc.ld
	dd if=shell.bin of=os_copy.img conv=notrunc bs=1 count=2048 seek=17858560
	dd if=shell.bin of=os.img conv=notrunc bs=1 count=2048 seek=17858560
	./add_file.o

clean:
	rm */*.o
