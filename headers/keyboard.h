#include "scancode.h"
void input(char *buffer, int length, int screen_index);
void init_keyboard(void);
void add_keyboard_event(scan_code_set3 scan_code, void *event_handler);
