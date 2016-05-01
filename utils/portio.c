#include "../headers/portio.h"

unsigned char inb(unsigned short port){ //in byte
	unsigned char result;
	asm("in al, dx" : "=a" (result) : "d" (port));
	return result;
}

void outb(unsigned short port, unsigned char value){ //out byte
	asm("out dx, al" : : "d" (port), "a" (value));	
}

unsigned short inw(unsigned short port){ //in word
	unsigned short result;
	asm("in ax, dx" : "=a" (result) : "d" (port));
	return result;
}

void outw(unsigned short port, unsigned short value){ //out word
	asm("out dx, ax" : : "d" (port), "a" (value));	
}
