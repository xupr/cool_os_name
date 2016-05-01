#include "../headers/exception.h"
#include "../headers/interrupts.h"
#include "../headers/screen.h"
#include "../headers/string.h"
#include "../headers/process.h"

void *exception_interrupt_entry;

void init_exception(void){ //initialize exceptions
	create_IDT_descriptor(0xE, (int)&exception_interrupt_entry, 0x8, 0x8E);
	print("--page faults initialized\n");
}

void exception_handler(int fault_info){ //handle page faults
	asm("pusha");
	void *address;
	asm("mov eax, cr2" : "=a"(address));
	handle_page_fault(address, fault_info);
	asm("popa");
}
