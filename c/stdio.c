#include "headers/stdio.h"
#include "../headers/system_call.h"
#include "headers/string.h"

void print(char *str){
	asm("int 0x40" : : "d"(str), "a"(PRINT));
}

void input(char *str, int length){
	asm("int 0x40" : : "d"(str), "a"(INPUT), "c"(length));
}

FILE open(char *file_name){
	FILE fd;
	asm("int 0x40" : "=a"(fd) : "a"(OPEN), "d"(file_name));	
	return fd;
}

void write(FILE fd, char *buff, int length){
	asm("int 0x40" : : "a"(WRITE), "b"(fd), "d"(buff), "c"(length));
}

void read(FILE fd, char *buff, int length){
	asm("int 0x40" : : "a"(READ), "b"(fd), "d"(buff), "c"(length));
}
