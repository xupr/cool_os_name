[BITS 32]
GLOBAL ata_interrupt_entry
EXTERN ata_interrupt_handler

SECTION .text
ata_interrupt_entry:
	CALL ata_interrupt_handler
	IRET
