[BITS 32]
GLOBAL _start
EXTERN k_main
SECTION .text
_start:
	POP EAX
	POP EDX
	MOV ESP, 0x5FFFFF
	PUSH EDX
	PUSH EAX
	CALL k_main
	STI
wait_for_interrupt:
	hlt
	JMP wait_for_interrupt
