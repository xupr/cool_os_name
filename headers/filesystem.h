typedef int FILE;

void init_filesystem(void);
void get_inode(char *file_name, void *buff);
int get_file_size(char *file_name);
FILE open(char *file_name);
int write(FILE file_descriptor_index, char *buff, int count);
int read(FILE file_descriptor_index, char *buff, int count);
void execute(char *file_name, int screen_index, int argc, char **argv);
void seek(FILE file, int new_file_offset);
void close(FILE file_descriptor_index);
