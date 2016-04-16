typedef int FILE;

void print(char *str);
void input(char *str, int length);

FILE fopen(char *file_name);
void fwrite(char *buff, int length, FILE fd);
void fread(char *buff, int length, FILE fd);
void fclose(FILE fd);

