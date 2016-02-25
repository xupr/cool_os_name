typedef int FILE;

void print(char *str);
void input(char *str, int length);

FILE open(char *file_name);
void write(FILE file_descriptor_index, char *buff, int count);
void read(FILE file_descriptor_index, char *buff, int count);

