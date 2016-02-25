[BITS 32]
EXTERN system_call_interrupt
GLOBAL system_call_iterrupt_entry

SECTION .text
system_call_iterrupt_entry:
	PUSHA
	CALL system_call_interrupt
	POPA
	IRET
