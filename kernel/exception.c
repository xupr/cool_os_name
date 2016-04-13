#include "../headers/exception.h"
#include "../headers/interrupts.h"
#include "../headers/screen.h"
#include "../headers/string.h"
#include "../headers/process.h"

void *exception_interrupt_entry;

void init_exception(void){
	//print("hello world! time to get an exception");i
//	print(itoa(exception_interrupt_entry));
	create_IDT_descriptor(0xE, (int)&exception_interrupt_entry, 0x8, 0x8F);
	print("--page faults initialized\n");
}

void exception_handler(int fault_info){
	asm("pusha");
	void *address;
//	asm("pop eax" : "=a"(fault_info));
	asm("mov eax, cr2" : "=a"(address));
	handle_page_fault(address, fault_info);
//	print(itoa(address));
	asm("popa");
}
