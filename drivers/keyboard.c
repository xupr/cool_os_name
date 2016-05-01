#include "../headers/scancode.h"
#include "../headers/keyboard.h"
#include "../headers/interrupts.h"
#include "../headers/screen.h"
#include "../headers/portio.h"
#include "../headers/list.h"
#include "../headers/heap.h"
#include "../headers/string.h"

#define KEYBOARD_BUFF_SIZE 1024

#define PS2_DATA_REGISTER 0x60
#define PS2_STATUS_REGISTER 0x64
#define PS2_COMMAND_REGISTER 0x64
#define PS2_GET_CONFIGURATION 0x20
#define PS2_SET_CONFIGURATION 0x60
#define PS2_SCAN_CODE 0xF0

typedef struct {
	char *keyboard_buffer;
	int keyboard_buffer_index;
	int need_to_buffer;
} keyboard_buffer_descriptor;

void *keyboard_interrupt_entry;
list *keyboard_buffers;
char need_to_buffer = 0;
static int status = 0;
static key_state state = KEY_DOWN;

list *KEYBOARD_BEHAVIOR[256];
scan_code_set3 COMMAND_KEYS[5] = {0, 0, 0, 0, 0};

void input(char *buffer, int length, int screen_index){ //get input from keyboard
	keyboard_buffer_descriptor *keyboard_buffer = (keyboard_buffer_descriptor *)get_list_element(keyboard_buffers, screen_index);
	keyboard_buffer->need_to_buffer = 1;
	while(keyboard_buffer->keyboard_buffer_index < length && keyboard_buffer->need_to_buffer)
		asm("hlt");

	memcpy(buffer, keyboard_buffer->keyboard_buffer, keyboard_buffer->keyboard_buffer_index);
	buffer[keyboard_buffer->keyboard_buffer_index] = '\0';
	keyboard_buffer->keyboard_buffer_index = 0;
}

void buffer_char(scan_code_set3 scan_code, key_state state){ //buffer a char if needed
	keyboard_buffer_descriptor *keyboard_buffer = (keyboard_buffer_descriptor *)get_list_element(keyboard_buffers, get_current_screen_index());
	if(keyboard_buffer->need_to_buffer){
		if(state == KEY_UP) //don't do anything on key release
			return;

		switch(COMMAND_KEYS[0]){
			case SCS3_LSHIFT:;
			case SCS3_RSHIFT:;
				char c = scancode_set3[scan_code];
				if(c > 0x60 && c < 0x7B)
					c -= 0x20;
				else if(c > 0x26 && c < 0x3E)
					c = SHIFT_ASCII[c-0x27];
				else if(c > 0x5A && c < 0x5E)
					c = SHIFT_ASCII[c-0x44];
				keyboard_buffer->keyboard_buffer[keyboard_buffer->keyboard_buffer_index] = c;
				++keyboard_buffer->keyboard_buffer_index;
				break;
			case 0:;
				keyboard_buffer->keyboard_buffer[keyboard_buffer->keyboard_buffer_index] = scancode_set3[scan_code];
				++keyboard_buffer->keyboard_buffer_index;
				if(scancode_set3[scan_code] == '\n')
					keyboard_buffer->need_to_buffer = 0;
				break;
		}
		return;
	}
}

void print_char(scan_code_set3 scan_code, key_state state){ //print a char
	if(state == KEY_UP) //don't do anything on key release
		return;

	char s[2] = {0, 0};
	switch(COMMAND_KEYS[0]){
		case SCS3_LSHIFT:;
		case SCS3_RSHIFT:;
			char c = scancode_set3[scan_code];
			if(c > 0x60 && c < 0x7B)
				c -= 0x20;
			//if(c > 0x)
			else if(c > 0x26 && c < 0x3E)
				c = SHIFT_ASCII[c-0x27];
			else if(c > 0x5A && c < 0x5E)
				c = SHIFT_ASCII[c-0x44];
			s[0] = c;
			set_vga_colors(WHITE, BLACK);
			print(s);
			break;
		case 0:;
			s[0] = scancode_set3[scan_code];
			set_vga_colors(WHITE, BLACK);
			print(s);
			break;
	}
	return;
}

void keyboard_command_char(scan_code_set3 scan_code, key_state state){ //add/remove a command char to the keyboard commands buffer
	int i = 0;
	if(state == KEY_DOWN){
		while(COMMAND_KEYS[i++] != 0);
		COMMAND_KEYS[i-1] = scan_code;
	}else{
		while(COMMAND_KEYS[i++] != scan_code);
		COMMAND_KEYS[i-1] = 0;
	}
	return;
}

void switch_screens_command(scan_code_set3 scan_code, key_state state){ //switch screens if needed
	if(state == KEY_DOWN)
		return;
	
	if((COMMAND_KEYS[0] == SCS3_LCTRL && COMMAND_KEYS[1] == SCS3_LALT) || (COMMAND_KEYS[1] == SCS3_LCTRL && COMMAND_KEYS[0] == SCS3_LALT)){
		switch(scan_code){
			case SCS3_1:
				switch_screen(0);
				break;
			case SCS3_2:
				switch_screen(1);
				break;
			case SCS3_3:
				switch_screen(2);
				break;
			case SCS3_4:
				switch_screen(3);
				break;
		}
	}
}

void keyboard_scroll_command(scan_code_set3 scan_code, key_state state){ //scroll single lines
	if(state == KEY_DOWN){
		switch(scan_code){
			case SCS3_UP:
				scroll_lines(-1);
				break;
			case SCS3_DOWN:
				scroll_lines(1);
				break;
			case SCS3_PAGEUP:
				scroll_pages(-1);
				break;
			case SCS3_PAGEDOWN:
				scroll_pages(1);
				break;
		}
	}
}

void keyboard_backspace_command(scan_code_set3 scan_code, key_state state){ //scroll multiple lines
	if(state != KEY_DOWN)
		return;
	keyboard_buffer_descriptor *keyboard_buffer = (keyboard_buffer_descriptor *)get_list_element(keyboard_buffers, get_current_screen_index());
	if(keyboard_buffer->need_to_buffer && keyboard_buffer->keyboard_buffer_index != 0){
		print("\b");
		keyboard_buffer->keyboard_buffer[--keyboard_buffer->keyboard_buffer_index] = '\0';
	}
}

void init_keyboard_behavior(void){ //add keyboard events to relevent chars
	int i;
	for(i = 0; i < 256; KEYBOARD_BEHAVIOR[i++] = create_list());
	add_keyboard_event(SCS3_A, print_char);
	add_keyboard_event(SCS3_B, print_char);
	add_keyboard_event(SCS3_C, print_char);
	add_keyboard_event(SCS3_D, print_char);
	add_keyboard_event(SCS3_E, print_char);
	add_keyboard_event(SCS3_F, print_char);
	add_keyboard_event(SCS3_G, print_char);
	add_keyboard_event(SCS3_H, print_char);
	add_keyboard_event(SCS3_I, print_char);
	add_keyboard_event(SCS3_J, print_char);
	add_keyboard_event(SCS3_K, print_char);
	add_keyboard_event(SCS3_L, print_char);
	add_keyboard_event(SCS3_M, print_char);
	add_keyboard_event(SCS3_N, print_char);
	add_keyboard_event(SCS3_O, print_char);
	add_keyboard_event(SCS3_P, print_char);
	add_keyboard_event(SCS3_Q, print_char);
	add_keyboard_event(SCS3_R, print_char);
	add_keyboard_event(SCS3_S, print_char);
	add_keyboard_event(SCS3_T, print_char);
	add_keyboard_event(SCS3_U, print_char);
	add_keyboard_event(SCS3_V, print_char);
	add_keyboard_event(SCS3_W, print_char);
	add_keyboard_event(SCS3_X, print_char);
	add_keyboard_event(SCS3_Y, print_char);
	add_keyboard_event(SCS3_Z, print_char);
	add_keyboard_event(SCS3_0, print_char);
	add_keyboard_event(SCS3_1, print_char);
	add_keyboard_event(SCS3_2, print_char);
	add_keyboard_event(SCS3_3, print_char);
	add_keyboard_event(SCS3_4, print_char);
	add_keyboard_event(SCS3_5, print_char);
	add_keyboard_event(SCS3_6, print_char);
	add_keyboard_event(SCS3_7, print_char);
	add_keyboard_event(SCS3_8, print_char);
	add_keyboard_event(SCS3_9, print_char);
	add_keyboard_event(SCS3_TILDA, print_char);
	add_keyboard_event(SCS3_EQUALS, print_char);
	add_keyboard_event(SCS3_MINUS, print_char);
	add_keyboard_event(SCS3_BACKSLASH, print_char);
	add_keyboard_event(SCS3_SPACE, print_char);
	add_keyboard_event(SCS3_OPENBRACE, print_char);
	add_keyboard_event(SCS3_CLOSEBRACE, print_char);
	add_keyboard_event(SCS3_SEMICOLON, print_char);
	add_keyboard_event(SCS3_APOSTROPHE, print_char);
	add_keyboard_event(SCS3_COMMA, print_char);
	add_keyboard_event(SCS3_DOT, print_char);
	add_keyboard_event(SCS3_SLASH, print_char);
	add_keyboard_event(SCS3_ENTER, print_char);
	
	add_keyboard_event(SCS3_A, buffer_char);
	add_keyboard_event(SCS3_B, buffer_char);
	add_keyboard_event(SCS3_C, buffer_char);
	add_keyboard_event(SCS3_D, buffer_char);
	add_keyboard_event(SCS3_E, buffer_char);
	add_keyboard_event(SCS3_F, buffer_char);
	add_keyboard_event(SCS3_G, buffer_char);
	add_keyboard_event(SCS3_H, buffer_char);
	add_keyboard_event(SCS3_I, buffer_char);
	add_keyboard_event(SCS3_J, buffer_char);
	add_keyboard_event(SCS3_K, buffer_char);
	add_keyboard_event(SCS3_L, buffer_char);
	add_keyboard_event(SCS3_M, buffer_char);
	add_keyboard_event(SCS3_N, buffer_char);
	add_keyboard_event(SCS3_O, buffer_char);
	add_keyboard_event(SCS3_P, buffer_char);
	add_keyboard_event(SCS3_Q, buffer_char);
	add_keyboard_event(SCS3_R, buffer_char);
	add_keyboard_event(SCS3_S, buffer_char);
	add_keyboard_event(SCS3_T, buffer_char);
	add_keyboard_event(SCS3_U, buffer_char);
	add_keyboard_event(SCS3_V, buffer_char);
	add_keyboard_event(SCS3_W, buffer_char);
	add_keyboard_event(SCS3_X, buffer_char);
	add_keyboard_event(SCS3_Y, buffer_char);
	add_keyboard_event(SCS3_Z, buffer_char);
	add_keyboard_event(SCS3_0, buffer_char);
	add_keyboard_event(SCS3_1, buffer_char);
	add_keyboard_event(SCS3_2, buffer_char);
	add_keyboard_event(SCS3_3, buffer_char);
	add_keyboard_event(SCS3_4, buffer_char);
	add_keyboard_event(SCS3_5, buffer_char);
	add_keyboard_event(SCS3_6, buffer_char);
	add_keyboard_event(SCS3_7, buffer_char);
	add_keyboard_event(SCS3_8, buffer_char);
	add_keyboard_event(SCS3_9, buffer_char);
	add_keyboard_event(SCS3_TILDA, buffer_char);
	add_keyboard_event(SCS3_EQUALS, buffer_char);
	add_keyboard_event(SCS3_MINUS, buffer_char);
	add_keyboard_event(SCS3_BACKSLASH, buffer_char);
	add_keyboard_event(SCS3_SPACE, buffer_char);
	add_keyboard_event(SCS3_OPENBRACE, buffer_char);
	add_keyboard_event(SCS3_CLOSEBRACE, buffer_char);
	add_keyboard_event(SCS3_SEMICOLON, buffer_char);
	add_keyboard_event(SCS3_APOSTROPHE, buffer_char);
	add_keyboard_event(SCS3_COMMA, buffer_char);
	add_keyboard_event(SCS3_DOT, buffer_char);
	add_keyboard_event(SCS3_SLASH, buffer_char);
	add_keyboard_event(SCS3_ENTER, buffer_char);

	add_keyboard_event(SCS3_LSHIFT, keyboard_command_char);
	add_keyboard_event(SCS3_RSHIFT, keyboard_command_char);
	add_keyboard_event(SCS3_LCTRL, keyboard_command_char);
	add_keyboard_event(SCS3_LALT, keyboard_command_char);	
	
	add_keyboard_event(SCS3_UP, keyboard_scroll_command);
	add_keyboard_event(SCS3_DOWN, keyboard_scroll_command);
	add_keyboard_event(SCS3_PAGEUP, keyboard_scroll_command);
	add_keyboard_event(SCS3_PAGEDOWN, keyboard_scroll_command);
	add_keyboard_event(SCS3_BACKSPACE, keyboard_backspace_command);
	
	add_keyboard_event(SCS3_1, switch_screens_command);
	add_keyboard_event(SCS3_2, switch_screens_command);
	add_keyboard_event(SCS3_3, switch_screens_command);
	add_keyboard_event(SCS3_4, switch_screens_command);
	return;
}

void add_keyboard_event(scan_code_set3 scan_code, void *event_handler){ //add keyboard event to a char
	add_to_list(KEYBOARD_BEHAVIOR[scan_code], event_handler);
	return;
}

void init_keyboard(void){ //initialize the keyboards
	keyboard_buffers = create_list();
	int i;
	for(i = 0; i < 4; ++i){
		keyboard_buffer_descriptor *keyboard_buffer = (keyboard_buffer_descriptor *)malloc(sizeof(keyboard_buffer_descriptor));
		keyboard_buffer->keyboard_buffer = (char *)malloc(sizeof(char)*KEYBOARD_BUFF_SIZE);
		keyboard_buffer->keyboard_buffer_index = 0;
		keyboard_buffer->need_to_buffer = 0;
		add_to_list(keyboard_buffers, keyboard_buffer);
	}
	init_keyboard_behavior();
	create_IDT_descriptor(0x21, (unsigned int) &keyboard_interrupt_entry, 0x8, 0x8E);
	outb(PS2_COMMAND_REGISTER, PS2_GET_CONFIGURATION);
	unsigned char ps2_configuration = inb(PS2_DATA_REGISTER); 
	outb(PS2_COMMAND_REGISTER, PS2_SET_CONFIGURATION);
	outb(PS2_DATA_REGISTER, ps2_configuration & ~0b1000000); //disable scan code translation
	status = 1;
	outb(PS2_DATA_REGISTER, PS2_SCAN_CODE); //set scan code to set 3
	return;
}

void keyboard_interrupt(void){ //handle keyboard interrupts
	if(status == 1){
		if(inb(PS2_DATA_REGISTER) == 0xFA){	
			status = 2;
			outb(PS2_DATA_REGISTER, 3); //set scan code to set 3
		}
	}else if(status == 2){
		if(inb(PS2_DATA_REGISTER) == 0xFA)
			status = 3;
	}else{
		unsigned char c = inb(PS2_DATA_REGISTER);
		if(c == 0xF0)
			state = KEY_UP;
		else{
			int i = 0;
			list_node *key_event = KEYBOARD_BEHAVIOR[c]->first;
			while(key_event != 0){
				((void (*)(scan_code_set3, key_state))key_event->value)(c, state);
				key_event = key_event->next;
			}
			state = KEY_DOWN;
		}
	}
	send_EOI(1);
	return;
}
