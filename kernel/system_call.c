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
			set_vga_colors(WHITE, BLACK);
			print_to_other_screen(str, get_process_screen_index(get_current_process()));
		//	print(str);
			break;

		case INPUT:
			asm("" : "=d"(str), "=c"(length));
			input(str, length, get_process_screen_index(get_current_process()));
			break;

		case HEAP_START:
			heap = get_heap_start(get_current_process()); 
			return (void *)&heap;
			break;
		
		case FOPEN:
			asm("" : "=d"(str));
			fd = fopen(str);
			return &fd;
		//	asm("pop eax;popa");
		//	asm("" : : "a"(fd));
		//	asm("iret");
			break;

		case FWRITE:
			asm("" : "=c"(length), "=d"(str), "=b"(fd));
			fwrite(str, length, fd);
			break;

		case FREAD:
			asm("" : "=c"(length), "=d"(str), "=b"(fd));
			fread(str, length, fd);
			break;

		case FCLOSE:
			asm("" : "=b"(fd));
			fclose(fd);
			break;
	}

	return 0;
	//print();
}
