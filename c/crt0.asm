
[BITS 32]
GLOBAL _start
SECTION .load
EXTERN init_heap
EXTERN main
EXTERN exit
_start:
;	MOV byte [0x100000], 3
	CALL init_heap
	CALL main
	PUSH EAX
	CALL exit
	ADD ESP, 4
	RET
