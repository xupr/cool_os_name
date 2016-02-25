nasm -f bin -o boot_stage1.bin boot.asm
#nasm -g -f bin -o kernel.bin kernel.asm
nasm -g -f elf -o boot_stage2asm.bin boot_stage2.asm
i686-elf-gcc -ffreestanding -masm=intel -c -o boot_stage2c.bin boot_stage2.c
i686-elf-ld -T link2.ld -o kernel.bin boot_stage2asm.bin boot_stage2c.bin
#i686-elf-ld -T link.ld -o boot.bin boot_stage1.bin
#i686-elf-ld -Ttext 0x10000 --oformat binary -o kernel.bin boot_stage2asm.bin boot_stage2c.bin

dd if=boot_stage1.bin of=boot.img bs=512
dd if=kernel.bin of=boot.img bs=512 oflag=append conv=notrunc
