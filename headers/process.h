#include "../headers/filesystem.h"
#define KERNEL_PID 0

typedef int PID;

void init_process(void);
FILE fopen(char *file_name);
int fread(char *buff, int count, FILE fd);
int fwrite(char *buff, int count, FILE fd);
void handle_page_fault(void *address, int fault_info);
void create_process(char *code, int length);
PID get_current_process(void);
void *get_heap_start(PID process_index);
