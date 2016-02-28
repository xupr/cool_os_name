[BITS 32]
GLOBAL _start
EXTERN init
SECTION .text
_start:
;	MOV byte [0x100000], 3
	CALL init
	RET
