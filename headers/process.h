#define KERNEL_PID 0

typedef int PID;

void init_process(void);
void create_process(char *code, int length);
PID get_current_process(void);
void *get_heap_start(PID process_index);
