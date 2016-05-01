#include "../headers/string.h"
#include "../headers/heap.h"
#include "../headers/screen.h"

//same string manipulation functions from the standard c library
char *itoa(unsigned int number){
	int length = 1;
	int _number = number;
	while((_number = _number/10) != 0) ++length;
	char *string = (char *)malloc((length + 1)*sizeof(char));
	*(string+length) = '\0';
	for(;length > 0; *(string+(--length)) = (number%10 + 0x30), number /= 10); 
	return string;
}

char strcmp(char *str1, char *str2){
	while(*(str1) != 0 && *(str2) != 0 && *(str1) == *(str2)){
		++str1;
		++str2;
	}
	if(*(str1) == 0 && *(str2) == 0) return 0;
	return *str1 - *str2;
}

void strcpy(char *str1, char *str2){
	while(*str2 != '\0') *(str1++) = *(str2++);
	*str1 = '\0';
}

void *memcpy(void *dst, void *src, int count){
	void *temp_dst = dst;
	while(count--) *(char *)dst++ = *(char *)src++;
	return temp_dst;
}

void *memset(void *dst, char data, int count){
	void *temp_dst = dst;
	while(count--) *(char *)dst = data;
	return temp_dst;
}

int strlen(char *str){
	char *_str = str;
	while(*str++); 
	return str - _str - 1;
}

char *strtok(char *str, char *separators){
	static char *current_str;
	if(str)
		current_str = str;
	else
		str = current_str;

	while(*current_str++){
		int i;
		for(i = strlen(separators) - 1; i >= 0; --i){
			if(*current_str == separators[i]){
				*current_str++ = '\0';
				return str;
			}
		}
	}
	--current_str;

	if(current_str == str)
		return 0;
	return str;
}
