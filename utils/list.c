#include "../headers/heap.h"
#include "../headers/screen.h"
#include "../headers/list.h"
#include "../headers/string.h"

list *create_list(void){
	list *lst = (list *)malloc(sizeof(list));
	lst->first = 0;
	lst->length = 0;
	return lst;
}

void add_to_list(list *lst, void *value){
	list_node *new_node = (list_node *)malloc(sizeof(list_node));
	new_node->value = value;
	new_node->next = 0;
	if(!lst->first){
		lst->first = new_node;
		lst->length = 1;
		return;
	}
	list_node *current = lst->first;
	while(current->next != 0) current = current->next;
	current->next = new_node;
	++lst->length;
	return;
}

void add_to_list_at(list *lst, void *value, int index){
	if(index > lst->length - 1){
		add_to_list(lst, value);
		return;
	}

	list_node *new_node = (list_node *)malloc(sizeof(list_node));
	new_node->value = value;
	++lst->length;
	if(index == 0){
		new_node->next = lst->first;
		lst->first = new_node;
		return;
	}

//	print("safta is love\n");
//	print(itoa(lst->length));
	list_node *temp_node = lst->first;
	--index;
	for(; index--; temp_node = temp_node->next);
//	print(itoa((int)(temp_node->next->next)));
	new_node->next = temp_node->next;
	temp_node->next = new_node;
}

void remove_from_list(list *lst, void *value){
	list_node *current = lst->first;
	if(lst->length == 0)
		return;
	if(current->value == value){
		lst->first = current->next;
		free(current);
		--lst->length;
		return;
	}
	list_node *prev = current;
	current = current->next;
	while(current != 0 && current->value != value){
		prev = current;
		current = current->next;
	}
	if(current){
		prev->next = current->next;
		free(current);
		--lst->length;
	}
	return;
}

void *get_list_element(list *lst, unsigned int index){
	if(index >= lst->length) return 0;
	list_node *current = lst->first;
	for(; index--; current = current->next);
	return current->value;
}
