#include "../headers/system_call.h"
#include "../headers/interrupts.h"
#include "../headers/screen.h"
#include "../headers/keyboard.h"
#include "../headers/process.h"
#include "../headers/filesystem.h"

void *system_call_iterrupt_entry;

void init_system_call(void){
	create_IDT_descriptor(0x40, (unsigned int)&system_call_iterrupt_entry, 0x8, 0xEF);
//	print("kappa123");
}

void *system_call_interrupt(void){
	enum SYSTEM_CALL system_call_name;
	asm("" : "=a"(system_call_name));
	char *str;
	int length;
	static void *heap;
	static FILE fd;
	switch(system_call_name){
		case PRINT:
			asm("" : "=d"(str));
			print(str);
			break;

		case INPUT:
			asm("" : "=d"(str), "=c"(length));
			input(str, length);
			break;

		case HEAP_START:
			heap = get_heap_start(get_current_process()); 
			return (void *)&heap;
			break;
		
		case OPEN:
			asm("" : "=d"(str));
			fd = open(str);
			return &fd;
		//	asm("pop eax;popa");
		//	asm("" : : "a"(fd));
		//	asm("iret");
			break;

		case WRITE:
			asm("" : "=c"(length), "=d"(str), "=b"(fd));
			write(fd, str, length);
			break;

		case READ:
			asm("" : "=c"(length), "=d"(str), "=b"(fd));
			read(fd, str, length);
			break;

	}

	return 0;
	//print();
}
