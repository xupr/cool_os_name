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
	static int bytes;
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
			return &heap;
			break;
		
		case FOPEN:;
			char *mode;
			asm("" : "=b"(mode), "=d"(str));
			fd = fopen(str, mode);
			return &fd;
		//	asm("pop eax;popa");
		//	asm("" : : "a"(fd));
		//	asm("iret");
			break;

		case FWRITE:
			asm("" : "=c"(length), "=d"(str), "=b"(fd));
			bytes = fwrite(str, length, fd);
			return &bytes;
			break;

		case FREAD:
			asm("" : "=c"(length), "=d"(str), "=b"(fd));
			bytes = fread(str, length, fd);
			return &bytes;
			break;

		case FCLOSE:
			asm("" : "=b"(fd));
			fclose(fd);
			break;

		case EXIT:;
			int status_code;
			asm("" : "=d"(status_code));
			exit_process(status_code);
			break;

		case EXECUTE:
			asm("" : "=d"(str));
			execute_from_process(str);
			break;

		case DUMP_MEMORY_MAP:
			dump_memory_map();
			break;
			
		case DUMP_PROCESS_LIST:
			dump_process_list();
			break;

		case GET_FILE_SIZE:
			asm("" : "=d"(str));
			bytes = get_file_size(str);
			return &bytes;
			break;

		case SETEUID:;
			int new_euid;
			asm("" : "=b"(new_euid));
			bytes = seteuid(new_euid);
			return &bytes;
	}

	return 0;
	//print();
}
