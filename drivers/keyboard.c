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

void *keyboard_interrupt_entry;
char *keyboard_buffer;
int keyboard_buffer_index = 0;
char need_to_buffer = 0;
static int status = 0;
static key_state state = KEY_DOWN;

//void (*KEYBOARD_BEHAVIOR[256][2])(scan_code_set3 scan_code, key_state state);
list *KEYBOARD_BEHAVIOR[256];
scan_code_set3 COMMAND_KEYS[5] = {0, 0, 0, 0, 0};

void input(char *buffer, int length){
	need_to_buffer = 1;
	//print("kappa123");
/*	print(itoa(keyboard_buffer_index));
	print("\n");
	print(itoa(length));
	print("\n");
*/	while(keyboard_buffer_index < length && need_to_buffer);//{
	//	if(keyboard_buffer_index > 0 && keyboard_buffer[keyboard_buffer_index - 1] == '\n')
	//		break;
	//}
	//need_to_buffer = 0;
	//print("kappa123");
	memcpy(buffer, keyboard_buffer, keyboard_buffer_index);
	buffer[keyboard_buffer_index] = '\0';
	keyboard_buffer_index = 0;
}

void buffer_char(scan_code_set3 scan_code, key_state state){
	if(need_to_buffer){
		if(state == KEY_UP) //don't do anything on key release
			return;

		//char s[2] = {0, 0};
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
				//s[0] = c;
				keyboard_buffer[keyboard_buffer_index] = c;
				++keyboard_buffer_index;
				//print(s);
				break;
			case 0:;
				keyboard_buffer[keyboard_buffer_index] = scancode_set3[scan_code];
				++keyboard_buffer_index;
				if(scancode_set3[scan_code] == '\n')//{
					need_to_buffer = 0;
//					print("kappa123?\n");
//				}
				//s[0] = scancode_set3[scan_code];
				//print(s);
				break;
		}
		return;
	}
}

void print_char(scan_code_set3 scan_code, key_state state){
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
			print(s);
			break;
		case 0:;
			s[0] = scancode_set3[scan_code];
			print(s);
			break;
	}
	return;
}

void keyboard_command_char(scan_code_set3 scan_code, key_state state){
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

void keyboard_scroll_command(scan_code_set3 scan_code, key_state state){
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

void init_keyboard_behavior(void){
	int i;
	for(i = 0; i < 256; KEYBOARD_BEHAVIOR[i++] = create_list())i;
	/*KEYBOARD_BEHAVIOR[SCS3_A][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_B][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_C][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_D][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_E][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_F][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_G][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_H][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_I][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_J][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_K][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_L][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_M][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_N][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_O][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_P][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_Q][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_R][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_S][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_T][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_U][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_W][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_X][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_Y][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_Z][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_0][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_1][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_2][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_3][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_4][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_5][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_6][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_7][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_8][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_9][0] = print_char;	
	KEYBOARD_BEHAVIOR[SCS3_TILDA][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_MINUS][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_EQUALS][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_BACKSLASH][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_BACKSPACE][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_SPACE][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_OPENBRACE][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_CLOSEBRACE][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_SEMICOLON][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_APOSTROPHE][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_COMMA][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_DOT][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_SLASH][0] = print_char;
	KEYBOARD_BEHAVIOR[SCS3_ENTER][0] = print_char;

	KEYBOARD_BEHAVIOR[SCS3_LSHIFT][0] = keyboard_command_char;
	KEYBOARD_BEHAVIOR[SCS3_RSHIFT][0] = keyboard_command_char;*/
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
	add_keyboard_event(SCS3_UP, keyboard_scroll_command);
	add_keyboard_event(SCS3_DOWN, keyboard_scroll_command);
	add_keyboard_event(SCS3_PAGEUP, keyboard_scroll_command);
	add_keyboard_event(SCS3_PAGEDOWN, keyboard_scroll_command);
	return;
}

void add_keyboard_event(scan_code_set3 scan_code, void *event_handler){
	//KEYBOARD_BEHAVIOR[scan_code][1] = event_handler;
	add_to_list(KEYBOARD_BEHAVIOR[scan_code], event_handler);
	return;
}

void init_keyboard(void){
	keyboard_buffer = (char *)malloc(sizeof(char)*KEYBOARD_BUFF_SIZE);
	init_keyboard_behavior();
	create_IDT_descriptor(0x21, (unsigned int) &keyboard_interrupt_entry, 0x8, 0x8F);
	outb(PS2_COMMAND_REGISTER, PS2_GET_CONFIGURATION);
	unsigned char ps2_configuration = inb(PS2_DATA_REGISTER); 
	outb(PS2_COMMAND_REGISTER, PS2_SET_CONFIGURATION);
	outb(PS2_DATA_REGISTER, ps2_configuration & ~0b1000000); //disable scan code translation
	status = 1;
	outb(PS2_DATA_REGISTER, PS2_SCAN_CODE); //set scan code to set 3
	return;
}

void keyboard_interrupt(void){
	//print("shmolik");
	//inb(PS2_STATUS_REGISTER);
	//inb(PS2_DATA_REGISTER);
	//outb(PS2_DATA_REGISTER, 0);
	//scancode_set3[0];
	if(status == 1){
		if(inb(PS2_DATA_REGISTER) == 0xFA){	
			status = 2;
			outb(PS2_DATA_REGISTER, 3); //set scan code to set 3
		}
	}else if(status == 2){
		if(inb(PS2_DATA_REGISTER) == 0xFA){
			status = 3;
			//outb(PS2_DATA_REGISTER, 0xF0);
		}
	/*}else if(status == 5){
		char scan_code[2] = {0x30 + inb(PS2_DATA_REGISTER), 0};
		print(scan_code);
		status = 0;
	}else if(status == 3){
		if(inb(PS2_DATA_REGISTER) == 0xFA){
			print("a");		
			status = 4;
			outb(PS2_DATA_REGISTER, 0);
		}
	}else if(status == 4){
		if(inb(PS2_DATA_REGISTER) == 0xFA){
			print("c");
			status = 5;
			//outb(PS2_DATA_REGISTER, 0xF0);
		}*/
	}else{
	/*char c = inb(PS2_DATA_REGISTER);
	if(c == 0xF0)
		c = inb(PS2_DATA_REGISTER);
	if(c == 0x1C)
		print("a");
	else{
		char s[2] = {c, 0};
		print(s);
	}*/
		unsigned char c = inb(PS2_DATA_REGISTER);
		if(c == 0xF0)
			state = KEY_UP;
			//print("doda");
		else{
		//	char s[2] = {scancode_set3[c], 0};
		//	print(s);
			int i = 0;
			//for(; i < 2 && KEYBOARD_BEHAVIOR[c][i] != 0; (KEYBOARD_BEHAVIOR[c][i++])(c, state));
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
