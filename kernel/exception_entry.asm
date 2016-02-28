[BITS 32]
GLOBAL exception_interrupt_entry
EXTERN exception_handler
SECTION .text
exception_interrupt_entry:
	CALL exception_handler
	ADD ESP, 4
	IRET
