#pragma once
#include "../headers/filesystem.h"
#define KERNEL_PID 0

typedef int PID;

void init_process(void);
FILE fopen(char *file_name, char *mode);
DIR opendir_from_process(char *file_name);
int fread(char *buff, int count, FILE fd);
int readdir_from_process(char *buff, int count, DIR dd);
int fwrite(char *buff, int count, FILE fd);
void fclose(FILE fd);
void closedir_from_process(DIR dd);
void exit_process(int result_code);
void execute_from_process(char *file_name, int argc, char **argv);
void handle_page_fault(void *address, int fault_info);
void create_process(char *code, int length, int screen_index, char *file_name, int argc, char **argv);
PID get_current_process(void);
int get_process_screen_index(PID process_index);
void *get_heap_start(PID process_index);
void dump_process_list(void);
int get_current_euid(void);
