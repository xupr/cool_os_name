#include "../c/headers/stdlib.h"
#include "../c/headers/stdio.h"
#include "../c/headers/string.h"
#include "../c/headers/os.h"

int main(void){
	/*char *username = "a";
	char *password = "a";*/
	while(1){
			/*execute("shell.bin");*/
		print("username: ");
		char input_username[32];
		input(input_username, 32);
		strtok(input_username, "\n");
		print("password: ");
		char input_password[32];
		input(input_password, 32);
		strtok(input_password, "\n");
		FILE fd = fopen("users.txt", "r");
		int bytes = get_file_size("users.txt");
		char *buff = (char *)malloc(bytes);
		fread(buff, bytes, fd);
		fclose(fd);
		char *username = strtok(buff, ":");
		int uid = 0;
		while(username){
			if(!strcmp(username, input_username)){
				char *password = strtok(0, ":");
				if(!strcmp(password, input_password)){
					seteuid(uid);
					execute("shell.bin");
					seteuid(0);
				}else
					print("wrong login credentials\n");
				break;
			}
			
			char *tmp = strtok(0, "\n");
			if(!tmp){
				print("wrong login credentials\n");
				break;
			}
			username = strtok(0, ":");
			++uid;
		}
		/*if(!strcmp(username, strtok(input_password, "\n")) && !strcmp(password, strtok(input_password, "\n")))
			execute("shell.bin");
		else
			print("wrong\n");*/
	}
}
