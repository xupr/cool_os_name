
[BITS 32]
GLOBAL _start
SECTION .load
EXTERN init_heap
EXTERN main
EXTERN exit
_start: ;initilize the heap, run main, then exit the process
	CALL init_heap
	CALL main
	PUSH EAX
	CALL exit
	ADD ESP, 4
	RET
