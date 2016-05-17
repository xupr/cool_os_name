#include "./headers/os.h"
#include "./headers/stdio.h"
#include "../headers/system_call.h"
#include "./headers/stat.h"

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

int stat(char *file_name, struct stat *buff){
	int success;
	asm("int 0x40" : "=a"(success): "a"(STAT), "d"(file_name), "b"(buff));
	return success;
}

DIR opendir(char *file_name){
	DIR dd;
	asm("int 0x40" : "=a"(dd): "a"(OPENDIR), "d"(file_name));
	return dd;
}

int readdir(char *buff, int count, DIR dd){
	int success;
	asm("int 0x40" : "=a"(success): "a"(READDIR), "d"(buff), "c"(count), "b"(dd));
	return success;
}

void closedir(DIR dd){
	asm("int 0x40" :: "a"(CLOSEDIR), "b"(dd));
}

FILE dup(FILE oldfd){
	FILE fd;
	asm("int 0x40" : "=a"(fd): "a"(DUP), "d"(oldfd));
	return fd;
}

FILE dup2(FILE oldfd, FILE newfd){
	FILE fd;
	asm("int 0x40" : "=a"(fd): "a"(DUP2), "d"(oldfd), "b"(newfd));
	return fd;
}
