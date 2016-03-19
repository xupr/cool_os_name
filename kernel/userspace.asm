[BITS 32]
GLOBAL jump_to_ring3
SECTION .text
jump_to_ring3:
	MOV EAX, [ESP + 4]
	MOV [address], EAX
	MOV AX, 0x23
	MOV DS, AX
	MOV ES, AX
	MOV FS, AX
	MOV GS, AX

	MOV EAX, 0x9FFFFF
	PUSH 0x23
	PUSH EAX
	;CLI
	PUSHF
	PUSH 0x1B
	PUSH DWORD [address]
	IRET

SECTION .data
address dd 0
