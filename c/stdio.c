#include "headers/stdio.h"
#include "../headers/system_call.h"
#include "headers/string.h"

void print(char *str){
	asm("int 0x40" : : "d"(str), "a"(PRINT));
}

void input(char *str, int length){
	asm("int 0x40" : : "d"(str), "a"(INPUT), "c"(length));
}

FILE fopen(char *file_name){
	FILE fd;
	asm("int 0x40" : "=a"(fd) : "a"(FOPEN), "d"(file_name));	
	return fd;
}

void fwrite(char *buff, int length, FILE fd){
	asm("int 0x40" : : "a"(FWRITE), "b"(fd), "d"(buff), "c"(length));
}

void fread(char *buff, int length, FILE fd){
	asm("int 0x40" : : "a"(FREAD), "b"(fd), "d"(buff), "c"(length));
}
