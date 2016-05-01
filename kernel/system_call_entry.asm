[BITS 32]
EXTERN system_call_interrupt
GLOBAL system_call_iterrupt_entry

SECTION .text
system_call_iterrupt_entry:
	PUSHA
	CALL system_call_interrupt ;call the system call handler
	TEST EAX, EAX ;check if there is return data, if so fetch it and return it, else return
	JZ no_return_data
	MOV EAX, [EAX]
	MOV [return_data], EAX
	POPA
	MOV EAX, [return_data]
	IRET
no_return_data:
	POPA
	IRET
SECTION .data
return_data dd 0
