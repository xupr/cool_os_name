[BITS 32]
GLOBAL pit_interrupt_entry
EXTERN pit_interrupt_handler
EXTERN jump_to_ring3
SECTION .text
pit_interrupt_entry:
	PUSHA
	PUSH ESP
	CALL pit_interrupt_handler
	ADD ESP, 4
	TEST EAX, EAX
	JE exit_pit_interrupt_entry
	MOV [save_eax], EAX
	POPA
	PUSH DWORD [save_eax]
	CALL jump_to_ring3

exit_pit_interrupt_entry:
	POPA
	IRET

SECTION .data
save_eax dd 0
