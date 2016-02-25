[BITS 16]
[ORG 0x7c00]	;Tells the assembler that its a 16 bit code	;Origin, tell the assembler that where the code will
GLOBAL BOOT

%define ATA_DATA_REGISTER_PORT 0x1F0
%define ATA_SECTOR_COUNT_REGISTER_PORT 0x1F2
%define ATA_LBA_LOW_REGISTER_PORT 0x1F3
%define ATA_LBA_MID_REGISTER_PORT 0x1F4
%define ATA_LBA_HIGH_REGISTER_PORT 0x1F5
%define ATA_DRIVE_REGISTER_PORT 0x1F6
%define ATA_COMMAND_REGISTER_PORT 0x1F7
%define ATA_REGULAR_STATUS_REGISTER_PORT 0x1F7

%define ATA_READ 0x20

SECTION .boot			;be in memory after it is been loaded
BOOT:
MOV AX, CS
MOV DS, AX
MOV SS, AX
MOV ES, AX
MOV AX, 0X7C0
ADD AX, 0X20
MOV SP, AX
MOV SI, HelloString ;Store string pointer to SI
CALL PrintString	;Call print string procedure.

MOV DI, memory_map
CALL get_memory_map
MOV [memory_map_length], AX

;CALL LoadKernel
MOV AX, 0X2401
INT 0X15
JC A20Failed
MOV SI, HelloString ;Store string pointer to SI
CALL PrintString	;Call print string procedure.

XOR EAX, EAX
MOV AX, DS
SHL EAX, 4
ADD EAX, gdt
MOV [offset], AX
LGDT [gdt_description_structure]

MOV EAX, CR0
OR AL, 1
MOV CR0, EAX

JMP 0x8:flush_selectors

[BITS 32]
flush_selectors:
MOV AX, 0x10
MOV DS, AX
MOV SS, AX
MOV ES, AX
MOV GS, AX
MOV FS, AX

CALL load_kernel
[BITS 32]
PUSH memory_map
PUSH memory_map_length
JMP 0x100000

[BITS 16]
A20Failed:
JMP $


PrintCharacter:	;Procedure to print character on screen
	;Assume that ASCII value is in register AL
MOV AH, 0x0E	;Tell BIOS that we need to print one charater on screen.
MOV BH, 0x00	;Page no.
MOV BL, 0x07	;Text attribute 0x07 is lightgrey font on black background

INT 0x10	;Call video interrupt
RET		;Return to calling procedure



PrintString:	;Procedure to print string on screen
	;Assume that string starting pointer is in register SI

next_character:	;Lable to fetch next character from string
MOV AL, [SI]	;Get a byte from string and store in AL register
INC SI		;Increment SI pointer
OR AL, AL	;Check if value in AL is zero (end of string)
JZ exit_function ;If end then return
CALL PrintCharacter ;Else print the character which is in AL register
JMP next_character	;Fetch next character from string
exit_function:	;End label
RET		;Return from procedure

;LoadKernel:
;	MOV AH, 0 ;clear floppy
;	MOV DL, 0

	;INT 0X13
	;JC LoadKernel

;retry:
;	MOV AX, 0x1000 ;address
;	MOV ES, AX 
;	XOR BX, BX ;address ES:BX
;	MOV AH, 0x2 
;	MOV AL, 128 ;sector count
;	MOV CH, 0 ;cylinder
;	MOV CL, 2 ;sector number
;	MOV DH, 0 ;head number
;	MOV DL, 0x80 ;drive number
;
;	INT 0x13
;	JC retry
;	;PUSH ES
;	;PUSH BX
;	;RETF
;	RET
;failed:
;	MOV SI, Fail
;	CALL PrintString
;
;	RET

load_kernel:
	[BITS 32]
	MOV AL, 0xE0
	MOV DX, ATA_DRIVE_REGISTER_PORT	
	OUT DX, AL
	
	MOV AL, 0
	MOV DX, ATA_SECTOR_COUNT_REGISTER_PORT
	OUT DX, AL

	MOV AL, 1
	MOV DX, ATA_LBA_LOW_REGISTER_PORT
	OUT DX, AL
	
	XOR AL, AL
	MOV DX, ATA_LBA_MID_REGISTER_PORT
	OUT DX, AL
	
	MOV DX, ATA_LBA_HIGH_REGISTER_PORT
	OUT DX, AL
	
	MOV AL, ATA_READ
	MOV DX, ATA_COMMAND_REGISTER_PORT
	OUT DX, AL
	
	MOV CX, 255
	MOV EDI, 0x100000
load_kernel_read_sectors:
	PUSH CX

	MOV DX, ATA_REGULAR_STATUS_REGISTER_PORT
load_kernel_test_BSY:
	IN AL, DX
	TEST AL, 0b10000000
	JNZ load_kernel_test_BSY
	
load_kernel_test_DRQ:
	IN AL, DX
	TEST AL, 0b1000
	JZ load_kernel_test_DRQ

	MOV DX, ATA_DATA_REGISTER_PORT
	MOV CX, 255
	REP INSW

	POP CX
	LOOP load_kernel_read_sectors
	
	RET
	[BITS 16]

get_memory_map:
	MOV EAX, 0
	PUSH EAX
	XOR BX, BX
get_memory_map_loop:
	CLC
	POP EAX
	INC EAX
	PUSH EAX
	MOV AX, 0xE820
	MOV EDX, 0x534D4150
	MOV CX, 24
	INT 0x15
	JC get_memory_map_error
	CMP EAX, 0x534D4150
	JNE get_memory_map_error
	CMP EBX, 0
	JE get_memory_map_exit
	ADD DI, CX
	JMP get_memory_map_loop

get_memory_map_error:
	MOV SI, Fail
	CALL PrintString

get_memory_map_exit:
	POP EAX
	RET

;Data

memory_map:
	TIMES 20*7 db 0

memory_map_length db 0

STRUC gdt_descriptor
	.limit_low resb 2
	.base_low resb 2
	.base_mid resb 1
	.access resb 1
	.flags_limit_high resb 1
	.base_high resb 1
ENDSTRUC 
gdt:
	TIMES 8 db 0

	istruc gdt_descriptor
		at gdt_descriptor.limit_low, dw 0xFFFF
		at gdt_descriptor.base_low, dw 0x0000
		at gdt_descriptor.base_mid, db 0x0
		at gdt_descriptor.access, db 0b10011010
		at gdt_descriptor.flags_limit_high, db 0b11001111
		at gdt_descriptor.base_high, db 0
	iend

	istruc gdt_descriptor
		at gdt_descriptor.limit_low, dw 0xFFFF
		at gdt_descriptor.base_low, dw 0x000
		at gdt_descriptor.base_mid, db 0x0
		at gdt_descriptor.access, db 0b10010010
		at gdt_descriptor.flags_limit_high, db 0b11001111
		at gdt_descriptor.base_high, db 0
	iend

;	istruc gdt_descriptor
;		at gdt_descriptor.limit_low, dw 0x1000
;		at gdt_descriptor.base_low, dw 0x0000
;		at gdt_descriptor.base_mid, db 0x3
;		at gdt_descriptor.access, db 0b10010110
;		at gdt_descriptor.flags_limit_high, db 0b01000000
;		at gdt_descriptor.base_high, db 0
;	iend

gdt_description_structure:
	size dw 23
	offset dd 0
HelloString db 'Yes This Is The Bootloader\n', 0	;HelloWorld string ending with 0
Fail db 'oops', 0


TIMES 510 - ($ - $$) db 0	;Fill the rest of sector with 0
DW 0xAA55			;Add boot signature at the end of bootloader

