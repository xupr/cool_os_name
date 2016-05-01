#include "../headers/interrupts.h"
#include "../headers/portio.h"

#define MASTER_PIC_COMMAND 0x20
#define MASTER_PIC_DATA 0x21
#define SLAVE_PIC_COMMAND 0xA0
#define SLAVE_PIC_DATA 0xA1

#define ICW1 0x10
#define ICW1_ICW4 0x1

#define ICW4_8086 0x1

#define MASTER_PIC_OFFSET 0x20
#define SLAVE_PIC_OFFSET 0x28

typedef struct __attribute__((__packed__)){
	unsigned short offset_low;
	unsigned short selector;
	unsigned char zero;
	unsigned char attr;
	unsigned short offset_high;
} IDT_descriptor;

static struct __attribute__((__packed__)){
	unsigned short limit;
	IDT_descriptor *base;
} *IDTR;

//void *keyboard_interrupt;

static IDT_descriptor *IDT_descriptors;

void create_IDT_descriptor(int index, unsigned int offset, unsigned short selector, unsigned char attr){ //create IDT descriptor
	*(IDT_descriptors + index) = (IDT_descriptor){.offset_low = offset & 0xFFFF, .selector = selector, .zero = 0, attr = attr, .offset_high = offset>>16};
}

void init_pic(void){ //initialize the PIC (moves the IRQs to different IDT selectors)
	unsigned char master_mask = inb(MASTER_PIC_DATA);
	unsigned char slave_mask = inb(SLAVE_PIC_DATA);

	outb(MASTER_PIC_COMMAND, ICW1 + ICW1_ICW4);
	outb(SLAVE_PIC_COMMAND, ICW1 + ICW1_ICW4);
	outb(MASTER_PIC_DATA, MASTER_PIC_OFFSET);
	outb(SLAVE_PIC_DATA, SLAVE_PIC_OFFSET);
	outb(MASTER_PIC_DATA, 4);
	outb(SLAVE_PIC_DATA, 2);
	outb(MASTER_PIC_DATA, ICW4_8086);
	outb(SLAVE_PIC_DATA, ICW4_8086);

	outb(MASTER_PIC_DATA, 0xfc);
	outb(SLAVE_PIC_DATA, slave_mask);
	return;
}

void init_interrupts(void){ //initialize interrupts
	init_pic();
	print("--PIC initialized\n");
	asm("SIDT [eax]" : "=a" (IDTR));
	IDT_descriptors = IDTR->base;
	print("--IDT initialized\n");
	
	return;
}

void send_EOI(int IRQ){ //send end of interrupt to allow the PIC to send the next interrupt
	outb(MASTER_PIC_COMMAND, 0x20);
	if(IRQ >= 8)
		outb(SLAVE_PIC_COMMAND, 0x20);
	return;
}
