typedef struct _list_node{
	void *value;
	struct _list_node *next;
} list_node;

typedef struct{
	list_node *first;
	int length;
} list;

list *create_list(void);
void add_to_list(list *lst, void *value);
void add_to_list_at(list *lst, void *value, int index);
void remove_from_list(list *lst, void *value);
void *get_list_element(list *lst, unsigned int index);
