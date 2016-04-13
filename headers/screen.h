#define SCREEN 0xB8000
#define SCREEN_END 0xD8000

typedef enum _VGA_COLOR {
	BLACK,
	BLUE,
	GREEN,
	CYAN,
	RED,
	MAGENTA,
	BROWN,
	GREY,
	DARK_GREY,
	BRIGHT_BLUE,
	BRIGHT_GREEN,
	BRIGHT_CYAN,
	BRIGHT_RED,
	BRIGHT_MAGENTA,
	YELLOW,
	WHITE
} VGA_COLOR;

int get_current_screen_index(void);
void switch_screen(int new_screen_index);
void print(char *str);
void print_to_other_screen(char *str, int screen_index);
void set_vga_colors(VGA_COLOR new_foreground, VGA_COLOR new_background);
void scroll_lines(int count);
void scroll_pages(int count);
void clear_screen(void);
void init_screen(void);
