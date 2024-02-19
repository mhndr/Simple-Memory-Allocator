//
//  main.c
//  Simple Memory Allocator
//
//  Created by Mahendar Srinivasan on 01/02/24.
//

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

char *heap_base=NULL;
char *heap_max=NULL;

typedef unsigned long  ulong;

#define CHUNK_SIZE 4096
#define WORD sizeof(ulong)
#define DWORD 2*WORD
#define MIN_BLK_SZ DWORD


#define SET_ALLOC(p) *(ulong*)p =  *(ulong*)p|0x1
#define SET_FREE(p) *(ulong*)p = *(ulong*)p & 0xFFFFFFFE

//TODO: if size is always a multiple of 8 and therefore
//the last three bits are always zero, do I still need to 
//do the left shift thrice??TODO
#define SET_SIZE(p,size) *(ulong*)p=(size)<<3
#define GET_SIZE(p) (*(ulong*)p&0xFFFFFFF8)>>3

#define IS_FREE(p) *(ulong*)p&0x1

//#define GET_HEADER(p) ((ulong*)p)--
//#define GET_FOOTER(p) (ulong*)p+GET_SIZE(GET_HEADER(p))-DWORD


int mem_init(void);
void* get(int size);
void free(void *ptr);
void print_blocks(void);
void print_heap(void);
void defragment_blocks(void);
void* coalesce_blocks(void *ptr);

int mem_init(void) {
    printf("mem_init\n");
	heap_base = (char*) mmap(NULL ,CHUNK_SIZE, PROT_READ|PROT_WRITE,MAP_ANON|MAP_PRIVATE,0,0);
    //heap_base = (char*) calloc(CHUNK_SIZE,sizeof(char));
    //heap_base = sbrk(CHUNK_SIZE);
    if(heap_base == NULL) {
        printf("Heap not allocated, can't proceed\n");
        return 0;
    }
    printf("heap allocated\n");
   	heap_max = heap_base + CHUNK_SIZE;
    
    SET_SIZE(heap_base,CHUNK_SIZE-WORD);
    SET_FREE(heap_base);
    char *footer = heap_base+CHUNK_SIZE;
    footer -= DWORD;
    SET_SIZE(footer,CHUNK_SIZE-WORD);
    SET_FREE(footer);

    
    int sz = GET_SIZE(heap_base);
    char *end = heap_base+sz;
	SET_SIZE(end,0);
    SET_ALLOC(end);
    printf("heap base=%p, heap max=%p, end block=%p, available size=%d",heap_base,heap_max,end,sz);

	return 1;
}

void print_heap(void) {
    printf("\n\nPRINTING HEAP\n");
    for(char *ptr = heap_base; ptr<heap_max;ptr+=sizeof(ulong))
    {
		if(*(int*)ptr !=0) 
        	printf("%p-%lu\n",ptr,GET_SIZE(ptr));
    }
}

void print_blocks(void) {
    char *ptr = heap_base;
	printf("\n\nPRINTING BLOCKS\n");
    int sz =  0; 
	int free=0;
	char *prev = NULL;
    while(1) {  
		sz = GET_SIZE(ptr);
		free = IS_FREE(ptr);	
        printf("block of size %d is %s at %p",sz,free==0?"free":"not free",ptr);
        if(sz==0 && free==1) {
            printf("\nfound end of last block\n");
            break;
        }
		ptr += sz;
		printf("-%p",ptr);
		prev = ptr-WORD;
        if('x'==getc(stdin))
			break;
	
    }
    
    //print reverse
    printf("\n\nPRINTING BLOCKS REVERSE\n");
    while(1) {
		sz = GET_SIZE(ptr); 
        free = IS_FREE(ptr);
 		printf("block of size %d is %s at %p",sz,free==0?"free":"not free",ptr);
		ptr -= sz;
		if(ptr+WORD==heap_base){
			printf("\nfound start of first block\n");
			break;
		}
		if(sz==0)
			ptr -= WORD; //prev block footer
		printf("-%p",ptr);
		if('x'==getc(stdin)) {
			break;
		}
    }
    return;
}

void* get(int size) {
    if(size<=0) {
        return NULL;
    }
    
    char *ptr = heap_base;
    char *payload = NULL;
    ulong blk_sz =  0; 
    int free = -1;
	int alloc_sz = size+DWORD;
	printf("\n\nSEARCHING FOR FREE BLOCK OF SIZE - %d",size);
 
	//TODO:Fix the padding logic   
/*    if(size<DWORD){
		alloc_sz = 2*DWORD;
	}
	else {
		alloc_sz = DWORD * (( size + (DWORD+ (DWORD-1)))/DWORD);
	}
	printf("\nreqested size:%d ,alloc size:%d ",size,alloc_sz);
	getc(stdin);
*/
	while(1) {
        blk_sz = GET_SIZE(ptr);
        free = IS_FREE(ptr);
        printf("\nblock of size %lu is %s at %p",blk_sz,free==0?"free":"not free",ptr);

		if(blk_sz == alloc_sz && free==0) {
			printf("\nfound free block of size %lu",blk_sz);
			return ptr+WORD;
		}
        
        else if(blk_sz==0 && free==1) {
            printf("\nERROR - no free block available");
            //expand heap and try again instead of returning NULL
            void *ret =  mmap(heap_base ,CHUNK_SIZE, PROT_READ|PROT_WRITE,MAP_ANON|MAP_PRIVATE,0,0);
            if(!ret) {
                printf("\nERROR - failed to expand heap");
                return NULL;
            }
            heap_max += CHUNK_SIZE;
            char *new = ptr;
			int new_blk_sz = CHUNK_SIZE-WORD;
            SET_SIZE(new,new_blk_sz); //create a new block of size CHUNK_SIZE
            SET_FREE(new);
            new += new_blk_sz;;
			new -= WORD;
			SET_SIZE(new,new_blk_sz); //footer
            SET_FREE(new);
			
			new += WORD;
            SET_SIZE(new,0); //move the end block
            SET_ALLOC(new);
            printf("\nSUCCESS - expanded heap");
			//coalesce the free block just before the heap expanded
			ptr = (char*)coalesce_blocks((void*)ptr); //ptr should've moved back if coalesce happened
			printf("\nSUCCESS - coalesced free blocks");
			printf("\nTrying to allocate %d bytes",size);
            continue;
        }
       
        //TODO: add padding to adjust blocks to nearest multiple of 8
		else if(blk_sz > alloc_sz && free==0) {
            if(blk_sz-alloc_sz < MIN_BLK_SZ) {
                //no need to fragment as block is too small.
                printf("\nallocating free block of size %lu",blk_sz);
                SET_ALLOC(ptr);
                return ptr+WORD;
            }
            printf("\nallocating %d bytes in free block of size %lu",alloc_sz,blk_sz);
            SET_SIZE(ptr,alloc_sz);
            SET_ALLOC(ptr);
            payload = ptr;
            ptr += alloc_sz;
			ptr -= WORD;
            SET_SIZE(ptr,alloc_sz);//footer
            SET_ALLOC(ptr);
 
			ptr += WORD; //update next block header
			ulong remainder = blk_sz-alloc_sz;
			printf("\ncreating new free block of size %lu at %p",remainder,ptr);
           	SET_SIZE(ptr,remainder);
            SET_FREE(ptr);
			ptr += remainder;//update next block footer
			ptr -= WORD;
			SET_SIZE(ptr,remainder);
            SET_FREE(ptr);
            return payload+WORD;	
        }
        ptr += blk_sz; //blk_sz < alloc_sz
    }
    
    //shouldn't reach here
    return NULL;
}

void* coalesce_blocks(void *ptr) {
	if(ptr==NULL) 
		return NULL;
	void *ret = NULL;
	char *start = (char *)ptr;
	char *curr = start;
	int size = 0;
	int total = 0;
	int count = 0;	

	printf("\n\nCOALESCE FREE BLOCKS\n");

	// forward check
	printf("looking forward starting at %p \n",curr);
	while(curr && (IS_FREE(curr))==0) {
		size = GET_SIZE(curr);
		printf("\tfound free block of size %d at %p\n",size,curr);
		total += size;
		curr += size;
		count += 1;
	}
	printf("found %d free blocks totalling %d bytes",count,total);
	
	if(count>1){
		SET_SIZE(ptr,total);
		SET_FREE(ptr);
		ptr += total;
		ptr -= WORD;	
		SET_SIZE(ptr,total);
		SET_FREE(ptr);
		count = 1;
	}

	if(start == heap_base) {
		return NULL;
	}
	
	// backward check
	printf("\nlooking backward starting at %p \n",curr);
	curr = start-WORD;//prev block header
	while(curr && (IS_FREE(curr))==0) {
		size = GET_SIZE(curr);
		printf("\tfound free block of size %d at %p\n",size,curr);
		total += size;
		curr -= size;
		count += 1;
		
		if(curr+WORD==heap_base){
			printf("found start of first block\n");
			break;
		}
	}
	printf("found %d free blocks totalling %d bytes\n",count,total);	

	if(count>1) {
		curr += WORD;
		printf("creating new free block of size %d at %p\n",total,curr);
		SET_SIZE(curr,total); // header of new coalesced block
		SET_FREE(curr);
		ret = curr; //set ptr to the newly coalesced mega block
		curr += total;
		curr -= WORD;
		SET_SIZE(curr,total); // footer of new coalesced block
		SET_FREE(curr);
	}
	return ret;   
}

void free(void *ptr) {
    if(ptr==NULL)
        return;

    char *hdr = (char*)ptr;
	char *ftr = NULL;
    int free = 0;
   	int sz = 0; 

	hdr = hdr-WORD;
	sz = GET_SIZE(hdr);
	ftr = (hdr + sz)-WORD;
   	
	free = IS_FREE(hdr);
	
	printf("\n\nFREE");
    if(free==0)
    {
        printf("\nERROR - attempting to free a freed block");
        //TODO: add exceptions on double free
        return;
    }
    SET_FREE(hdr);
	SET_FREE(ftr);
    printf("\nfreed block of size %d, header:%p-%lu footer:%p-%lu",sz,hdr,GET_SIZE(hdr),ftr,GET_SIZE(ftr));
	coalesce_blocks(hdr);
	return;
}


void mem_tests();
void test_small();
void test_large();
void test_double_free();
void test_leak();
void test_use_after_free();
void test_coalesce();

int main(void) {
    
    mem_init();
    print_heap();
    print_blocks();
    
    void* p1 = get(10);
	print_heap();
	print_blocks();
	void* p2 = get(200);
	print_blocks();
    void* p3 = get(500);
//	print_blocks();
    free(p1);
    free(p2);
    free(p3);
//    print_blocks();
/*	p1 = get(1);
    p2 = get(1);
    p3 = get(250);
	print_blocks();
    free(p1);
    free(p2);
    free(p3);
	print_blocks();
    p1=get(2000);
    p2=get(2000);
    p3=get(2000);
	print_blocks();
    free(p1);
    free(p2);
    free(p3);
*/	print_blocks();
//	p1=get(3000);
//    p2=get(3000);
//    p3=get(3000);
//    free(p1);
//    free(p2);
//    free(p3);
	p1=get(10000);
    p2=get(5000);
    p3=get(7000);
	print_blocks();
    free(p1);
    free(p2);
    free(p3);

    print_blocks();
    return 0;
}

