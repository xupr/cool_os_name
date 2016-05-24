#include "../c/headers/stdlib.h"
#include "../c/headers/stdio.h"
#include "../c/headers/string.h"
#include "../c/headers/os.h"
#include "../c/headers/stat.h"

int main(int argc, char **argv){ 
	char *write_file_commad = "write", //write text to a file, usage: write [filename] [text]
	     *read_file_command = "read", //reads text from a file, usage: read [filename]
	     *echo_command = "echo", //echos text back to the screen, usage: echo [text]
	     *exit_command = "exit", //exits the shell, usage: exit
	     *exec_command = "exec"; //executes a file, usage: exec [filename]
	
	char *out = (char *)malloc(1024*sizeof(char)), *str;
	str = (char *)malloc(1024*sizeof(char));
	struct stat *st_buff = (struct stat *)malloc(sizeof(struct stat));
	FILE _stdin = dup(stdin);
	FILE _stdout = dup(stdout);
	while(1){
		print("[");
		print(argv[0]);
		print("@cool_os_name]$ ");
		memset(str, 0, 1024);
		if(!fread(str, 1024, stdin))
			break;
		char *command = strtok(str, " \n");
		if(!command)
			continue;
		if(!strcmp(command, write_file_commad)){
			char *file_name = strtok(0, " \n");
			if(!file_name){
				print("file name needed\n");
				continue;
			}

			char *data = strtok(0, "\n");
			if(!data){
				print("text needed\n");
				continue;
			}

			FILE fd = fopen(file_name, "w");
			if(fd != -1){
				fwrite(data, strlen(data) + 1, fd);
				fclose(fd);
			}else
				print("no premisions\n");
		}else if(!strcmp(command, read_file_command)){
			char *file_name = strtok(0, " \n");
			if(!file_name){
				print("file name needed\n");
				continue;
			}

			FILE fd = fopen(file_name, "r");
			if(fd != -1){
				memset(out, 0, 1024);
				fread(out, 1024, fd);
				print(out);
				print("\n");
				fclose(fd);
			}else
				print("no premisions\n");
		}else if(!strcmp(command, echo_command)){
			print(strtok(0, "\n"));
			print("\n");
		}else if(!strcmp(command, exit_command)){
			break;
		}else if(!stat(command, st_buff)){
			int argc = 1;
			char **argv = (char **)malloc(sizeof(char *)*32);
			argv[0] = command;
			char *token;
			int err = 0;
			FILE newstdin = 0, newstdout = 0;
			while(token = strtok(0, " \n")){
				if(!strcmp(token, "<")){
					if(!(token = strtok(0, " \n"))){
						print("no file name specified");
						err = 1;	
						break;
					}

					FILE fd = fopen(token, "r");
					if(fd == -1){
						print("couldn't open ");
						print(token);
						err = 1;
						break;
					}

					newstdin = fd;
				}else if(!strcmp(token, ">")){
					if(!(token = strtok(0, " \n"))){
						print("no file name specified");
						err = 1;	
						break;
					}

					FILE fd = fopen(token, "w");
					if(fd == -1){
						print("couldn't open ");
						print(token);
						err = 1;
						break;
					}

					newstdout = fd;
				}else
					argv[argc++] = token;
			}

			if(err)
				continue;
			
			if(newstdout)
				dup2(newstdout, stdout);
			if(newstdin)
				dup2(newstdin, stdin);

			execute(command, argc, argv);
			if(newstdout){
				fclose(newstdout);
				dup2(_stdout, stdout);
			}if(newstdin){
				fclose(newstdin);
				dup2(_stdin, stdin);
			}
		}	
	}
	return 0;
}
