void create_IDT_descriptor(int index, unsigned int offset, unsigned short selector, unsigned char attr);
void init_interrupts(void);
void send_EOI(int IRQ);
