#include "../c/headers/stdlib.h"
#include "../c/headers/stdio.h"
#include "../c/headers/string.h"
#include "../c/headers/os.h"

int main(int argc, char *argv[]){
	//read get username and password from keyboard input and compares it to data from users.txt, if matches logs on to the user and opens a shell
	while(1){
		print("username: ");
		char input_username[32];
		fread(input_username, 32, stdin);
		strtok(input_username, "\n");
		print("password: ");
		char input_password[32];
		fread(input_password, 32, stdin);
		strtok(input_password, "\n");
		FILE fd = fopen("users.txt", "r");
		int bytes = get_file_size("users.txt");
		char *buff = (char *)malloc(bytes);
		fread(buff, bytes, fd);
		fclose(fd);
		char *username = strtok(buff, ":");
		int uid = 0;
		char **argv = (char **)malloc(sizeof(char *));
		while(username){
			if(!strcmp(username, input_username)){
				char *password = strtok(0, ":");
				if(!strcmp(password, input_password)){
					seteuid(uid);
					argv[0] = (char *)malloc(strlen(username) + 1);
					strcpy(argv[0], username);
					execute("shell", 1, argv);
					seteuid(0);
				}else
					print("wrong login credentials\n");
				break;
			}
			
			char *tmp = strtok(0, "\n");
			username = strtok(0, ":");
			if(!username)
				print("wrong login credentials\n");
			++uid;
		}
	}
}
