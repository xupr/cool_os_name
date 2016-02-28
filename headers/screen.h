#define SCREEN 0xB8000
#define SCREEN_END 0xD8000

void print(char *str);
void scroll_lines(int count);
void scroll_pages(int count);
void clear_screen(void);
void init_screen(void);
