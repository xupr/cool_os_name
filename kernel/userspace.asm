[BITS 32]
GLOBAL jump_to_ring3
SECTION .text
jump_to_ring3:
	MOV [save_eax], EAX
	;PUSHF
	;POP EAX
	MOV EAX, [ESP + 20]
	MOV [flags], EAX
	MOV EAX, [ESP + 8]
	MOV [stack_address], EAX
	MOV EAX, [ESP + 4]
	MOV [code_address], EAX
	MOV AX, 0x23
	MOV DS, AX
	MOV ES, AX
	MOV FS, AX
	MOV GS, AX
	MOV EAX, [save_eax]
	
	PUSH 0x23
	PUSH DWORD [stack_address]
	;PUSH 0x9FFFFF 
	;CLI
	PUSH DWORD [flags]
	PUSH 0x1B
	PUSH DWORD [code_address]
	IRET

SECTION .data
code_address dd 0
stack_address dd 0
flags dd 0
save_eax dd 0
