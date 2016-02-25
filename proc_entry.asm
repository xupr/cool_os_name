[BITS 32]
GLOBAL _start
EXTERN main
SECTION .text
_start:
	CALL main
	RET
