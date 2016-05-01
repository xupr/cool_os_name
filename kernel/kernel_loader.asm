[BITS 32]
GLOBAL _start
EXTERN k_main
SECTION .load
_start:
	POP EAX ;get the memory map from the bootloader
	POP EDX
	MOV ESP, 0x5FFFFF ;change the stack pointer
	PUSH EDX
	PUSH EAX
	CALL k_main ;call the kernel
	STI
wait_for_interrupt:
	HLT
	JMP wait_for_interrupt
