typedef int FILE;

void init_filesystem(void);
FILE open(char *file_name);
void write(FILE file_descriptor_index, char *buff, int count);
void read(FILE file_descriptor_index, char *buff, int count);
void execute(char *file_name);
void seek(FILE file, int new_file_offset);
