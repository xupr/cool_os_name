#include "../headers/portio.h"
#include "../headers/screen.h"

#define MISC_OUTPUT_REGISTER_READ 0x3CC
#define MISC_OUTPUT_REGISTER_WRITE 0x3C2
#define CRT_ADDRESS_REGISTER 0x3D4
#define CRT_DATA_REGISTER 0x3D5
#define CURSOR_LOCATION_HIGH_REGISTER_INDEX 0xE
#define CURSOR_LOCATION_LOW_REGISTER_INDEX 0xF
#define CURSOR_START_REGISTER_INDEX 0xA
#define CURSOR_END_REGISTER_INDEX 0xB
#define MAXIMUM_SCAN_LINE_REGISTER_INDEX 0x9
#define HORIZONTAL_TOTAL_REGISTER_INDEX 0x0
#define VERTICAL_TOTAL_REGISTER_INDEX 0x6
#define START_ADDRESS_LOW_REGISTER_INDEX 0xD
#define START_ADDRESS_HIGH_REGISTER_INDEX 0xC
#define ATTRIBUTE_ADDRESS_DATA_REGISTER 0x3C0
#define ATTRIBUTE_DATA_READ_REGISTER 0x3C1
#define ATTRIBUTE_MODE_CONTROL_REGISTER_INDEX 0x10
#define INPUT_STATUS1_REGISTER 0x3DA
#define PALETTE_ADDRESS_SOURCE 0x20

#define ROWS 25
#define COLUMNS 80

void init_cursor(void);
short get_cursor(void);
void set_cursor(short address);

static VGA_COLOR foreground = WHITE;
static VGA_COLOR background = GREEN;
static list *screen_list;

void init_screen(void){
	clear_screen();
	unsigned char mo_reg = inb(MISC_OUTPUT_REGISTER_READ);
	outb(MISC_OUTPUT_REGISTER_WRITE, mo_reg | 1); //set color graphics adapter addressing
	//outb(CRT_ADDRESS_REGISTER, HORIZONTAL_TOTAL_REGISTER_INDEX);
	//outb(CRT_DATA_REGISTER, 5)i;
	//enable bright colors and disable blinking
	inb(INPUT_STATUS1_REGISTER);
	outb(ATTRIBUTE_ADDRESS_DATA_REGISTER, ATTRIBUTE_MODE_CONTROL_REGISTER_INDEX | PALETTE_ADDRESS_SOURCE);
	char mode_control_register = inb(ATTRIBUTE_DATA_READ_REGISTER);
	outb(ATTRIBUTE_ADDRESS_DATA_REGISTER, mode_control_register & (~8));
	/*outb(CRT_ADDRESS_REGISTER, 0x11);
	unsigned char vertical_retrace_end_reg = inb(CRT_DATA_REGISTER);
	if(vertical_retrace_end_reg & 0b10000000) print("Protected");
	else print("unprotected");
	outb(CRT_DATA_REGISTER, vertical_retrace_end_reg & 0b0111111);
	
	vertical_retrace_end_reg = inb(CRT_DATA_REGISTER);
	if(vertical_retrace_end_reg & 0b10000000) print("Protected");
	else print("unprotected");

	outb(CRT_ADDRESS_REGISTER, HORIZONTAL_TOTAL_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, COLUMNS - 5); //set the columnts to 80

	outb(CRT_ADDRESS_REGISTER, VERTICAL_TOTAL_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, ROWS);*/

	//outb(CRT_ADDRESS_REGISTER, HORIZONTAL_TOTAL_REGISTER_INDEX);
	//inb(CRT_DATA_REGISTER);

	init_cursor();
	//set_cursor(5);
	return;
}

void set_vga_colors(VGA_COLOR new_foreground, VGA_COLOR new_background){
	foreground = new_foreground;
	background = new_background;
}

unsigned short get_start_address(){
	outb(CRT_ADDRESS_REGISTER, START_ADDRESS_LOW_REGISTER_INDEX);
	unsigned short offset = inb(CRT_DATA_REGISTER);
	
	outb(CRT_ADDRESS_REGISTER, START_ADDRESS_HIGH_REGISTER_INDEX);
	offset += inb(CRT_DATA_REGISTER)<<8;
	
	return offset;
}

void scroll_lines(int count){
	unsigned short offset = get_start_address() + count*COLUMNS;
	if(offset < 0)
		offset = 0;
	
	outb(CRT_ADDRESS_REGISTER, START_ADDRESS_LOW_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, offset & 0xFF);
	
	outb(CRT_ADDRESS_REGISTER, START_ADDRESS_HIGH_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, offset>>8);
}

void scroll_pages(int count){
	unsigned short offset = get_start_address() + count*ROWS*COLUMNS;
	if(offset < 0)
		offset = 0;
	
	outb(CRT_ADDRESS_REGISTER, START_ADDRESS_LOW_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, offset & 0xFF);
	
	outb(CRT_ADDRESS_REGISTER, START_ADDRESS_HIGH_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, offset>>8);
}

void print(char *str){
	unsigned char *screen = (unsigned char *)SCREEN + 2*get_cursor();
	char color = foreground + (background<<4);
	while(*str != '\0'){
		if(*str == '\n'){
			screen += 2*(COLUMNS-(((int)screen - (int)SCREEN)/2)%COLUMNS);
			++str;
		}else if(*str == '\b'){
			*--screen = '\0';
			*--screen = '\0';
			++str;
		}else{
			*screen++ = *str++;
			*screen++ = color;
		}
	}

	*screen++ = 0;
	*screen = 0x0f;
	
	//if(foreground != WHITE)
	foreground = WHITE;
	//if(background != BRIGHT_RED)
	background = GREEN;

	set_cursor(((int)screen - (int)SCREEN)/2);
	unsigned short offset = get_start_address();
	if(offset + ROWS*COLUMNS < ((int)screen - (int)SCREEN)/2)
		scroll_lines((((int)screen - (int)SCREEN)/2 - offset - ROWS*COLUMNS)/COLUMNS + 1);
	else if(offset > ((int)screen - (int)SCREEN)/2)
		scroll_lines((((int)screen - (int)SCREEN)/2 - offset)/COLUMNS - 1);
}

void clear_screen(void){
	unsigned char *screen = (unsigned char *)SCREEN;
	while(screen < (unsigned char *)SCREEN_END) *screen++ = '\0';
}

void init_cursor(void){
	unsigned char crt_address_reg = inb(CRT_ADDRESS_REGISTER); //save the crt address reg for later restoration
	
	outb(CRT_ADDRESS_REGISTER, CURSOR_START_REGISTER_INDEX); 
	outb(CRT_DATA_REGISTER, 0); //enable the cursor

	outb(CRT_ADDRESS_REGISTER, MAXIMUM_SCAN_LINE_REGISTER_INDEX);
	unsigned char max_scan_line_reg = inb(CRT_DATA_REGISTER); //get the maximum scan line

	outb(CRT_ADDRESS_REGISTER, CURSOR_END_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, max_scan_line_reg & 0b11111); //set cursor end to the maximum scan line
	
	//set cursor the the start of the screen
	outb(CRT_ADDRESS_REGISTER, CURSOR_LOCATION_LOW_REGISTER_INDEX); 
	outb(CRT_DATA_REGISTER, 0);
	
	outb(CRT_ADDRESS_REGISTER, CURSOR_LOCATION_HIGH_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, 0);

	outb(CRT_ADDRESS_REGISTER, crt_address_reg); //restore crt address register
	return;
}

short get_cursor(void){	
	unsigned char crt_address_reg = inb(CRT_ADDRESS_REGISTER); //save the crt address reg for later restoration

	outb(CRT_ADDRESS_REGISTER, CURSOR_LOCATION_LOW_REGISTER_INDEX);
	unsigned char cursor_low_address = inb(CRT_DATA_REGISTER); //get low address byte

	outb(CRT_ADDRESS_REGISTER, CURSOR_LOCATION_HIGH_REGISTER_INDEX);
	unsigned char cursor_high_address = inb(CRT_DATA_REGISTER); //get high address byte

	outb(CRT_ADDRESS_REGISTER, crt_address_reg); //restore crt address register
	return cursor_low_address + (cursor_high_address<<8);
}

void set_cursor(short address){	
	unsigned char crt_address_reg = inb(CRT_ADDRESS_REGISTER); //save the crt address reg for later restoration

	outb(CRT_ADDRESS_REGISTER, CURSOR_LOCATION_LOW_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, address & 0xFF); //set low address byte

	outb(CRT_ADDRESS_REGISTER, CURSOR_LOCATION_HIGH_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, address>>8); //set high address byte

	outb(CRT_ADDRESS_REGISTER, crt_address_reg); //restore crt address register
	return;
}
