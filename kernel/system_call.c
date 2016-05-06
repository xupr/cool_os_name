#include "../headers/system_call.h"
#include "../headers/interrupts.h"
#include "../headers/screen.h"
#include "../headers/keyboard.h"
#include "../headers/process.h"
#include "../headers/filesystem.h"

void *system_call_iterrupt_entry;

void init_system_call(void){ //initialize system call interupt
	create_IDT_descriptor(0x40, (unsigned int)&system_call_iterrupt_entry, 0x8, 0xEF);
}

void *system_call_interrupt(void){ //issue a system call according to the value in eax
	enum SYSTEM_CALL system_call_name;
	asm("" : "=a"(system_call_name));
	char *str;
	int length;
	static void *heap;
	static FILE fd;
	static DIR dd;
	static int bytes;
	switch(system_call_name){
		case PRINT:
			asm("" : "=d"(str));
			set_vga_colors(WHITE, BLACK);
			print_to_other_screen(str, get_process_screen_index(get_current_process()));
			break;

		case INPUT:
			asm("" : "=d"(str), "=c"(length));
			input(str, length, get_process_screen_index(get_current_process()));
			break;

		case HEAP_START:
			heap = get_heap_start(get_current_process()); 
			return &heap;
		
		case FOPEN:;
			char *mode;
			asm("" : "=b"(mode), "=d"(str));
			fd = fopen(str, mode);
			return &fd;

		case FWRITE:
			asm("" : "=c"(length), "=d"(str), "=b"(fd));
			bytes = fwrite(str, length, fd);
			return &bytes;

		case FREAD:
			asm("" : "=c"(length), "=d"(str), "=b"(fd));
			bytes = fread(str, length, fd);
			return &bytes;

		case FCLOSE:
			asm("" : "=b"(fd));
			fclose(fd);
			break;

		case EXIT:;
			int status_code;
			asm("" : "=d"(status_code));
			exit_process(status_code);
			break;

		case EXECUTE:;
			int argc;
			char **argv;
			asm("" : "=d"(str), "=c"(argc), "=b"(argv));
			execute_from_process(str, argc, argv);
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

		case SETEUID:;
			int new_euid;
			asm("" : "=b"(new_euid));
			bytes = seteuid(new_euid);
			return &bytes;

		case STAT:;
			void *buff;
			asm("" : "=d"(str), "=b"(buff));
			bytes = stat(str, buff);
			return &bytes;

		case OPENDIR:
			asm("" : "=d"(str));
			dd = opendir_from_process(str);
			return &dd;

		case READDIR:
			asm("" : "=d"(buff), "=c"(length), "=b"(dd));
			bytes = readdir_from_process(buff, length, dd);
			return &bytes;

		case CLOSEDIR:
			asm("" : "=b"(dd));
			closedir_from_process(dd);
			break;

	}

	return 0;
}
