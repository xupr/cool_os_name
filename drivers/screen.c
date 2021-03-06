#include "../headers/portio.h"
#include "../headers/screen.h"
#include "../headers/string.h"
#include "../headers/list.h"
#include "../headers/heap.h"

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

#define SCREEN_SIZE 131072

void init_cursor(void);
short get_cursor(void);
void set_cursor(short address);

typedef struct{
	char *screen;
	int cursor_offset;
	int scroll;
} screen_descriptor;

static VGA_COLOR foreground = WHITE;
static VGA_COLOR background = GREEN;
static list *screen_list;
static int current_screen_index = 0;
static int to_print = 1;
static int tab_size = 8;

int get_current_screen_index(void){ //returns the current screen index
	return current_screen_index;
}

void set_tab_size(int new_tab_size){ //sets the tab size when printing with \t
	tab_size = new_tab_size;
}

void init_screen(void){ //initializes the screen
	clear_screen();
	unsigned char mo_reg = inb(MISC_OUTPUT_REGISTER_READ);
	outb(MISC_OUTPUT_REGISTER_WRITE, mo_reg | 1); //set color graphics adapter addressing
	inb(INPUT_STATUS1_REGISTER);
	outb(ATTRIBUTE_ADDRESS_DATA_REGISTER, ATTRIBUTE_MODE_CONTROL_REGISTER_INDEX | PALETTE_ADDRESS_SOURCE);
	char mode_control_register = inb(ATTRIBUTE_DATA_READ_REGISTER);
	outb(ATTRIBUTE_ADDRESS_DATA_REGISTER, mode_control_register & (~8));

	init_cursor();
	screen_list = create_list();
	int i;
	for(i = 0; i < 4; ++i){
		screen_descriptor *sd = (screen_descriptor *)malloc(sizeof(screen_descriptor));
		sd->screen = (char *)malloc(SCREEN_SIZE);
		sd->cursor_offset = 0;
		sd->scroll = 0;
		memset(sd->screen, 0, SCREEN_SIZE);
		add_to_list(screen_list, sd);
	}
	return;
}

void switch_screen(int new_screen_index){ //switch to a different screen
	cli();
	screen_descriptor *current_sd = (screen_descriptor *)get_list_element(screen_list, current_screen_index);
	screen_descriptor *new_sd = (screen_descriptor *)get_list_element(screen_list, new_screen_index);
	memcpy(current_sd->screen, (char *)SCREEN, SCREEN_SIZE);
	current_sd->cursor_offset = get_cursor();
	memcpy((char *)SCREEN, new_sd->screen, SCREEN_SIZE);
	current_screen_index = new_screen_index;
	int scroll = new_sd->scroll - current_sd->scroll;
	scroll_lines(scroll);
	new_sd->scroll -= scroll;
	set_cursor(new_sd->cursor_offset);
	sti();
}

void set_vga_colors(VGA_COLOR new_foreground, VGA_COLOR new_background){ //set vga colors for next print
	if(!to_print)
		return;
	foreground = new_foreground;
	background = new_background;
}

unsigned short get_start_address(){ //get screen start address
	outb(CRT_ADDRESS_REGISTER, START_ADDRESS_LOW_REGISTER_INDEX);
	unsigned short offset = inb(CRT_DATA_REGISTER);
	
	outb(CRT_ADDRESS_REGISTER, START_ADDRESS_HIGH_REGISTER_INDEX);
	offset += inb(CRT_DATA_REGISTER)<<8;
	
	return offset;
}

void scroll_lines(int count){ //scroll lines forward or backward
	screen_descriptor *current_sd = (screen_descriptor *)get_list_element(screen_list, current_screen_index);
	current_sd->scroll += count;
	unsigned short offset = get_start_address() + count*COLUMNS;
	if(offset < 0)
		offset = 0;
	
	outb(CRT_ADDRESS_REGISTER, START_ADDRESS_LOW_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, offset & 0xFF);
	
	outb(CRT_ADDRESS_REGISTER, START_ADDRESS_HIGH_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, offset>>8);
}

void scroll_pages(int count){ //scroll whole pages
	screen_descriptor *current_sd = (screen_descriptor *)get_list_element(screen_list, current_screen_index);
	current_sd->scroll += count*ROWS;
	unsigned short offset = get_start_address() + count*ROWS*COLUMNS;
	if(offset < 0)
		offset = 0;
	
	outb(CRT_ADDRESS_REGISTER, START_ADDRESS_LOW_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, offset & 0xFF);
	
	outb(CRT_ADDRESS_REGISTER, START_ADDRESS_HIGH_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, offset>>8);
}

void print(char *str){ //print to current screen and move the cursor and scroll position accordingly
	if(!to_print)
		return;
	cli();
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
		}else if(*str == '\t'){
			screen += (tab_size - ((((int)screen - (int)SCREEN)/2)%COLUMNS)%tab_size)*2;
			++str;
		}else{
			*screen++ = *str++;
			*screen++ = color;
		}
	}

	*screen++ = 0;
	*screen = 0x0f;
	
	foreground = WHITE;
	background = GREEN;

	set_cursor(((int)screen - (int)SCREEN)/2);
	unsigned short offset = get_start_address();
	if(offset + ROWS*COLUMNS < ((int)screen - (int)SCREEN)/2)
		scroll_lines((((int)screen - (int)SCREEN)/2 - offset - ROWS*COLUMNS)/COLUMNS + 1);
	else if(offset > ((int)screen - (int)SCREEN)/2)
		scroll_lines((((int)screen - (int)SCREEN)/2 - offset)/COLUMNS - 1);
	sti();
}

void print_to_other_screen(char *str, int screen_index){ //print to a screen and alter it scroll position and cursur position accordingly
	if(!to_print)
		return;

	if(current_screen_index == screen_index){
		print(str);
		return;
	}
	cli();
	screen_descriptor *sd = get_list_element(screen_list, screen_index);
	unsigned char *screen = (unsigned char *)sd->screen + 2*sd->cursor_offset;
	int offset = (((int)screen - (int)sd->screen)/2 + strlen(str))/COLUMNS - sd->scroll - ROWS;
	if(offset > 0)
		sd->scroll += offset;
	char color = foreground + (background<<4);
	while(*str != '\0'){
		if(*str == '\n'){
			screen += 2*(COLUMNS-(((int)screen - (int)sd->screen)/2)%COLUMNS);
			++str;
		}else if(*str == '\b'){
			*--screen = '\0';
			*--screen = '\0';
			++str;
		}else if(*str == '\t'){
			*screen += (tab_size - ((((int)screen - (int)sd->screen)/2)%COLUMNS)%tab_size)*2;
			++str;
		}else{
			*screen++ = *str++;
			*screen++ = color;
		}
	}

	*screen++ = 0;
	*screen = 0x0f;
	
	foreground = WHITE;
	background = GREEN;
	
	sd->cursor_offset = ((int)screen - (int)sd->screen)/2;
	sti();
}

void print_on(void){ //turn printing on
	to_print = 1;
}

void print_off(void){ //turn printing off (debuging purposes)
	/*to_print = 0;*/
}

void clear_screen(void){ //clear the screen
	unsigned char *screen = (unsigned char *)SCREEN;
	while(screen < (unsigned char *)SCREEN_END) *screen++ = '\0';
}

void init_cursor(void){ //initialize the cursor
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

short get_cursor(void){	//get cursor position
	unsigned char crt_address_reg = inb(CRT_ADDRESS_REGISTER); //save the crt address reg for later restoration

	outb(CRT_ADDRESS_REGISTER, CURSOR_LOCATION_LOW_REGISTER_INDEX);
	unsigned char cursor_low_address = inb(CRT_DATA_REGISTER); //get low address byte

	outb(CRT_ADDRESS_REGISTER, CURSOR_LOCATION_HIGH_REGISTER_INDEX);
	unsigned char cursor_high_address = inb(CRT_DATA_REGISTER); //get high address byte

	outb(CRT_ADDRESS_REGISTER, crt_address_reg); //restore crt address register
	return cursor_low_address + (cursor_high_address<<8);
}

void set_cursor(short address){	//set the cursor position
	unsigned char crt_address_reg = inb(CRT_ADDRESS_REGISTER); //save the crt address reg for later restoration

	outb(CRT_ADDRESS_REGISTER, CURSOR_LOCATION_LOW_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, address & 0xFF); //set low address byte

	outb(CRT_ADDRESS_REGISTER, CURSOR_LOCATION_HIGH_REGISTER_INDEX);
	outb(CRT_DATA_REGISTER, address>>8); //set high address byte

	outb(CRT_ADDRESS_REGISTER, crt_address_reg); //restore crt address register
	return;
}
