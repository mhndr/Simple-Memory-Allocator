/*
Trying to create a very simple memory allocator. Just for learning
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

char *heap_base;
char *heap_top;
char *heap_max;

#define HEAP_SIZE 4096
#define WORD 4
#define DWORD 2*WORD

#define SET_ALLOC(p) *(unsigned int*)p =  *(int*)p|0x0001
#define SET_FREE(p) *(unsigned int*)p = *(int*)p & 0xFFFE

#define SET_SIZE(p,size) *(unsigned int*)p=(size)<<3
#define GET_SIZE(p) (*(unsigned int*)p&0xFFF8)>>3

#define IS_FREE(p) *(unsigned int*)p&0x1

#define GET_HEADER(p) ((unsigned int*)p)--

int mem_init(void);
void* get(int size);
void free(void *ptr);
void print_blocks(void);
void print_heap(void);
void defragment_blocks(void);

int mem_init(void) {
    heap_base = (char*) mmap(NULL ,HEAP_SIZE, PROT_READ|PROT_WRITE,MAP_ANON|MAP_PRIVATE,0,0);
    printf("mem_init\n");
    //heap_base = (char*) calloc(HEAP_SIZE,sizeof(char));
    //heap_base = sbrk(HEAP_SIZE);
    if(heap_base == NULL) {
        printf("Heap not allocated, can't proceed\n");
        return 0;
    }
    printf("heap allocated\n");
    heap_top = heap_base;
    heap_max = heap_base + HEAP_SIZE;
    
    SET_SIZE(heap_top,HEAP_SIZE-WORD);
    SET_FREE(heap_top);

    int sz = GET_SIZE(heap_top);
    char *end = heap_base+sz;
    printf("heap base=%p, heap max=%p, end ptr=%p,sz=%d",heap_base,heap_max,end,sz);
    SET_SIZE(end,0);
    SET_ALLOC(end);
    return 1;
}

void print_heap(void) {
    printf("\n\nPRINTING HEAP\n");
    for(char *ptr = heap_base; ptr<heap_max;ptr+=sizeof(int))
    {
        printf("%d",*(int*)ptr);
    }
    
}

void print_blocks(void) {
    char *ptr = heap_base;
    printf("\n\nPRINTING BLOCKS\n");
    int sz =  0;//should end up with the contiguous allocated size
    int free=0;
    while(1) {  
	sz = GET_SIZE(ptr);
	free = IS_FREE(ptr);	
        printf("block of size %d is %s at %p",sz,free==0?"free":"not free",ptr);
        if(sz==0 && free==1) {
            printf("\nfound last block");
            break;
        }
        if('x'==getc(stdin))
		break;
	ptr += sz;
	//ptr -=WORD;
    }
    return;
}

void* get(int size) {
    if(size<=0) {
        return NULL;
    }
    char *ptr = heap_base;
    char *payload = NULL;
    int blk_sz =  0;//should end up with the sum of all allocated block sizes
    printf("\n\nSEARCHING FOR FREE BLOCK OF SIZE- %d",size);
    while(1) { 
        blk_sz = GET_SIZE(ptr);	
	int free = IS_FREE(ptr);
    	printf("\nblock of size %d is %s at %p",blk_sz,free==0?"free":"not free",ptr);
        
    	if(blk_sz==0 && free==1) {
            printf("\nfound last block");
            break;
    	}
	if(blk_sz == size+WORD && free==0) {
	    printf("\nfound free block of size %d",blk_sz);
	    return ptr+WORD;
	}
       
        if(blk_sz > size+WORD && free==0) {
            printf("\nallocating %d bytes in free block of size %d",size+WORD,blk_sz);
            SET_SIZE(ptr,size+WORD);
            SET_ALLOC(ptr);
            payload = ptr;
            /*if(ptr+size==heap_max){
                return payload+WORD;
            }*/
            ptr += size;
	    ptr += WORD;
	    printf("\ncreating new free block of size %d at %p",blk_sz-size,ptr);
            SET_SIZE(ptr,blk_sz-(size+WORD));
            SET_FREE(ptr);
            return payload+WORD;
        }
        ptr += blk_sz;
    }
    if(heap_top+size>= heap_max) {
        return NULL;
    }
    printf("\nallocated heap size is %d ",blk_sz);
    SET_SIZE(heap_top,size);
    SET_ALLOC(heap_top);
    ptr = heap_top;
    heap_top +=size;
    SET_SIZE(heap_top,HEAP_SIZE-blk_sz);
                
    return ptr+WORD;
}

void defragment_blocks(void){
    
}

void free(void *ptr){
    char* hdr = (char*)ptr;
    int free = 0;
    hdr = hdr-WORD;
    free = IS_FREE(hdr);
    printf("\n\nFREE\nattempting to free %s block ",free==0?"a free":"an allocated");
    SET_FREE(hdr);
    printf("\nfreed block of size %d",GET_SIZE(hdr));
    return;
}

int main(void) {
    mem_init();
    print_blocks();
    void* p1 = get(10);
    void* p2 = get(200);
    void* p3 = get(500);
    free(p1);
    //free(p2);
    free(p3);
    print_blocks();
    p1 = get(1);
    //p2 = get(100);
    p3 = get(250);
    print_blocks();
    free(p1);
    free(p2);
    free(p3);
    print_blocks();
    return 0;
}
