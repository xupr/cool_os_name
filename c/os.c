#include "./headers/os.h"
#include "../headers/system_call.h"

void execute(char *file_name){
	asm("pusha");
	asm("int 0x40" : : "a"(EXECUTE), "d"(file_name));
	asm("popa");
}

void dump_memory_map(void){
	asm("pusha");
	asm("int 0x40" : : "a"(DUMP_MEMORY_MAP));
	asm("popa");
}

void dump_process_list(void){
	asm("pusha");
	asm("int 0x40" : : "a"(DUMP_PROCESS_LIST));
	asm("popa");
}

int get_file_size(char *file_name){
	int bytes;
	asm("int 0x40" : "=a"(bytes): "a"(GET_FILE_SIZE), "d"(file_name));
	return bytes;
}

int seteuid(int new_euid){
	int success;
	asm("int 0x40" : "=a"(success): "a"(SETEUID), "b"(new_euid));
	return success;
}
