typedef int DIR;

void execute(char *file_name, int argc, char **argv);
void dump_memory_map(void);
void dump_process_list(void);
int get_file_size(char *file_name);
int seteuid(int new_euid);
DIR opendir(char *file_name);
int readdir(char *buff, int count, DIR dd);
void closedir(DIR dd);
