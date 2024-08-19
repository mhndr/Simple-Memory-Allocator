/*
Implement LRU in software linked list.

Head1 ->Key:1|Value=5 ->Key:0|Value=10 ->Key:99|Value=12 ->Key:8|Value=13 ->'\0'

Read will bring the node to Head if found. If not found, return MISS (-1)
Linked List can grow till 20 nodes
Write/Add will cause the node to be added to the head. 
Least recently used node is removed so that node count does not exceed 20.
*/
#include<stdio.h>
#include<stdlib.h>

#define SIZE 5
#define MISS -1

typedef struct _node
{
	struct _node *next;
	int key;
	int value;
}node;


int Add(int key, int val);
int Read(int key);

extern void* get(int size);
extern void put(void *ptr);
extern int mem_init(void);
extern void print_blocks(void);

node *Head;
int size = 0;

int Read(int key)
{
	if(Head == NULL)
		return MISS;
	node *curr = Head;
	node *prev = NULL;
	if(Head->key == key)
		return Head->value;
	while(curr && curr->key!=key)
	{	
		prev = curr;
		curr = curr->next;
	}
	if(prev && curr)
	{
		//move the node to head
		prev->next = curr->next;
		curr->next = Head;
		Head = curr;
		return Head->value;
	}
	return MISS;	
}

node* create_node(int key,int val,node* next)
{
	node *new = (node*) get(sizeof(node));
	//print_blocks();;
	new->key = key;
	new->value = val;
	new->next = next;
	return new;
}

int Add(int key, int val)
{
	node *curr = Head;
	node *prev = NULL;

	if(!Head) //scenario -1 - first time init
	{
		Head = create_node(key,val,NULL); 
		size = 1;
		return 0;				
	}
	//scenario-3 - list is not empty but less than 20
	if(size < SIZE)
	{
		node *new = create_node(key,val,Head);
		Head = new;
		size++;
		return 0;
	}	
	//scenario-4 - list is not empty but equal to 20
	while(curr->next)
	{
		prev = curr;
		curr=curr->next;
	}
	printf("freeing node %d-%d",curr->key,curr->value);
	put(curr);
	print_blocks();
	prev->next = NULL;

	//insert at head
	node *new = create_node(key,val,Head);
	Head = new;
	return 0;
}	

void print_all()
{
	node *curr = Head;
	if(!curr)
		return;
	printf("\n");
	while(curr)
	{
		printf("%p->",curr);
		curr = curr->next;
	}

	curr = Head;
	printf("\n");
	while(curr)
	{
		printf("%d-%d->",curr->key,curr->value);
		curr = curr->next;
	}

	printf(" size-%d\n",size);
}	


int main(int argc, char **argv)
{
	int key;
	int val;
	mem_init();	
	while(1)
	{
		printf("\nEnter key - ");
		scanf("%d",&key);
		val = Read(key);
		if(val==MISS)
		{
			printf("\nEnter val - ");
			scanf("%d",&val);
			Add(key,val);
		}
		else
		{
			printf("val - %d",val);
		}
		print_all();
	}
}
/*
OUTPUT
mahendar@Mahendars-MacBook-Air C % ./a.out  

Enter key - 1

Enter val - 11

1-11-> size-1

Enter key -  
1
val - 11
1-11-> size-1

Enter key - 1
val - 11
1-11-> size-1

Enter key - 2

Enter val - 22

2-22->1-11-> size-2

Enter key - 3

Enter val - 33

3-33->2-22->1-11-> size-3

Enter key - 4

Enter val - 44

4-44->3-33->2-22->1-11-> size-4

Enter key - 5

Enter val - 55

5-55->4-44->3-33->2-22->1-11-> size-5

Enter key - 6

Enter val - 66
freeing node 1-11
6-66->5-55->4-44->3-33->2-22-> size-5

Enter key - 2
val - 22
2-22->6-66->5-55->4-44->3-33-> size-5

Enter key - 3
val - 33
3-33->2-22->6-66->5-55->4-44-> size-5

Enter key - 4
val - 44
4-44->3-33->2-22->6-66->5-55-> size-5

Enter key - 4
val - 44
4-44->3-33->2-22->6-66->5-55-> size-5

Enter key - 7

Enter val - 77
freeing node 5-55
7-77->4-44->3-33->2-22->6-66-> size-5

*/












