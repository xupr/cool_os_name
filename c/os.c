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
