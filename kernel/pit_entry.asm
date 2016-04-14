[BITS 32]
GLOBAL pit_interrupt_entry
EXTERN pit_interrupt_handler
EXTERN jump_to_ring3
SECTION .text
asd_pit_interrupt_entry:
	PUSHA
;	PUSHF
	PUSH ESP
	CALL pit_interrupt_handler
	ADD ESP, 4
	TEST EAX, EAX
	JE exit_pit_interrupt_entry
	MOV [save_eax], EAX
	MOV EAX, [ESP + 44]
	MOV [save_esp], EAX
;	POPF
	POPA
	PUSH DWORD [save_esp]
	PUSH DWORD [save_eax]
	CALL jump_to_ring3

pit_interrupt_entry:
	PUSHA
	PUSH ESP
	CALL pit_interrupt_handler
	ADD ESP, 4
	MOV EAX, [ESP + 36]
	AND EAX, 3
	TEST EAX, EAX
	JE exit_pit_interrupt_entry
	MOV AX, [ESP + 48]
	MOV DS, AX
	MOV ES, AX
	MOV FS, AX
	MOV GS, AX

exit_pit_interrupt_entry:
;	POPF
	POPA
	IRET

SECTION .data
save_eax dd 0
save_esp dd 0
