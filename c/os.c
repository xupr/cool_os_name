#include "./headers/os.h"
#include "../headers/system_call.h"

void execute(char *file_name, int argc, char **argv){ //execute a file
	asm("pusha");
	asm("int 0x40" : : "a"(EXECUTE), "d"(file_name), "c"(argc), "b"(argv));
	asm("popa");
}

void dump_memory_map(void){ //dump memory map
	asm("pusha");
	asm("int 0x40" : : "a"(DUMP_MEMORY_MAP));
	asm("popa");
}

void dump_process_list(void){ //dump process list
	asm("pusha");
	asm("int 0x40" : : "a"(DUMP_PROCESS_LIST));
	asm("popa");
}

int get_file_size(char *file_name){ //get the size of a file
	int bytes;
	asm("int 0x40" : "=a"(bytes): "a"(GET_FILE_SIZE), "d"(file_name));
	return bytes;
}

int seteuid(int new_euid){ //change euid
	int success;
	asm("int 0x40" : "=a"(success): "a"(SETEUID), "b"(new_euid));
	return success;
}
