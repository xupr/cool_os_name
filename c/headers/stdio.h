#pragma once
#define stdin 0
#define stdout 1

typedef int FILE;

void print(char *str);
void input(char *str, int length);

FILE fopen(char *file_name, char *mode);
int fwrite(char *buff, int length, FILE fd);
int fread(char *buff, int length, FILE fd);
void fclose(FILE fd);

