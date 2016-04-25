#include "../c/headers/stdlib.h"
#include "../c/headers/stdio.h"
#include "../c/headers/string.h"
#include "../c/headers/os.h"

int main(void){
	/*int i;
	for(i = 0; i < 10000; ++i);*/
	/*return 0;*/
	/*while(1) print("");*/
	char *write_file_commad = "write",
	     *read_file_command = "read",
	     *echo_command = "echo",
	     *exit_command = "exit",
	     *exec_command = "exec";
	
//	print(itoa((int)malloc(10)));
//	print(itoa((int)malloc(10)));
//	char *commands[] = {"write asd.txt hello\n", "read asd.txt\n", "write safta.txt world\n", "read safta.txt\n"};
//	int i;
	char *out = (char *)malloc(1024*sizeof(char)), *str;
/*	for(i = 0; i < 0; ++i){	
		print(">>> ");
		//char str[32];
		//char out[32];
		//input(str, 32);
		str = commands[i];
		print(str);
		char *command = strtok(str, " \n");
		if(!strcmp(command, write_file_commad)){
			//print("holy");
			char *file_name = strtok(0, " \n"),
			     *data = strtok(0, "\n");
			//print(file_name);
			FILE fd = open(file_name);
//			print(itoa(fd));
			write(fd, data, strlen(data) + 1);
		}else if(!strcmp(command, read_file_command)){
			char *file_name = strtok(0, " \n");
			FILE fd = open(file_name);
			read(fd, out, 32);
			print(out);
			print("\n");
		}	
	}
*/
	str = (char *)malloc(1024*sizeof(char));
	while(1){
		print(">>> ");
		memset(str, 0, 1024);
		//char out[32];
		input(str, 1024);
//		print("got my input");	
		char *command = strtok(str, " \n");
		if(!strcmp(command, write_file_commad)){
			//print("holy");
			char *file_name = strtok(0, " \n"),
			     *data = strtok(0, "\n");
			//print(file_name);
			FILE fd = fopen(file_name, "w");
			if(fd != -1){
				fwrite(data, strlen(data) + 1, fd);
				fclose(fd);
			}else
				print("no premisions\n");
		}else if(!strcmp(command, read_file_command)){
			char *file_name = strtok(0, " \n");
			FILE fd = fopen(file_name, "r");
			if(fd != -1){
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
		}else if(!strcmp(command, exec_command)){
			execute(strtok(0, "\n"));
		}	
		//	print("i'm so fking happy right now!");
	}
/*	char *str = "safta is love i said";
//	print(str);
	char *token = strtok(str, " ");
	while(token){
		print(token);
		print("\n");
		token = strtok(0, " ");
	}*/
	return 0;
}
