[BITS 32]
GLOBAL keyboard_interrupt_entry
EXTERN keyboard_interrupt

SECTION .text
keyboard_interrupt_entry:
	PUSHA
	CALL keyboard_interrupt
	POPA
	IRET

