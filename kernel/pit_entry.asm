[BITS 32]
GLOBAL pit_interrupt_entry
EXTERN pit_interrupt_handler
EXTERN jump_to_ring3
SECTION .text

pit_interrupt_entry:
	MOV [save_eax], EAX ;check if the interrupt came from user or kernel
	MOV EAX, [ESP + 4]
	TEST EAX, 3
	JNE no_need_to_pad_stack ;if it came from the kernel add 8 bytes bellow the interrupt data, else continue
	SUB ESP, 8
	MOV EAX, [ESP + 8]
	MOV [ESP], EAX
	MOV EAX, [ESP + 12]
	MOV [ESP + 4], EAX
	MOV EAX, [ESP + 16]
	MOV [ESP + 8], EAX
	MOV [ESP + 12], DWORD 0
	MOV [ESP + 16], DWORD 0

no_need_to_pad_stack:
	MOV EAX, [save_eax]
	PUSHA
	PUSH ESP
	CALL pit_interrupt_handler ;call the scheduler
	ADD ESP, 4
	MOV EAX, [ESP + 36] ;check if moving to a different ring
	TEST EAX, 3
	JE clean_stack_padding ;if returning to kernel clean 8 bytes from under the needed interrupt data, else change segment selector and return

	MOV AX, [ESP + 48]
	MOV DS, AX
	MOV ES, AX
	MOV FS, AX
	MOV GS, AX

	POPA
	IRET

clean_stack_padding:
	POPA
	MOV [save_eax], EAX
	CMP DWORD [ESP + 12], 0 ;check if the kernel stack need to be changed, if so change and return else clean and return
	JE no_need_to_change_kernel_stack
	MOV [save_esp], EBX
	MOV EBX, [ESP + 12] 
	
	MOV EAX, [ESP]
	MOV [EBX - 12], EAX
	MOV EAX, [ESP + 4]
	MOV [EBX - 8], EAX
	MOV EAX, [ESP + 8]
	MOV [EBX - 4], EAX
	MOV ESP, EBX
	SUB ESP, 12

	MOV EBX, [save_esp]
	MOV EAX, [save_eax]
	IRET

no_need_to_change_kernel_stack:
	MOV EAX, [ESP + 8]
	MOV [ESP + 16], EAX
	MOV EAX, [ESP + 4]
	MOV [ESP + 12], EAX
	MOV EAX, [ESP]
	MOV [ESP + 8], EAX
	ADD ESP, 8

	MOV EAX, [save_eax]
	IRET

SECTION .data
save_eax dd 0
save_esp dd 0
