/*
 * ECE454 Lab 3 - Malloc
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "aalwiudglauwidg",
    /* First member's first name and last name */
    "Jackson:Nie",
    /* First member's student number */
    "1005282409",
    /* Second member's first name and last name (do not modify if working alone) */
    "",
    /* Second member's student number (do not modify if working alone) */
    ""
};

/*************************************************************************
 * Basic Constants and Macros
 * You are not required to use these macros but may find them helpful.
*************************************************************************/
#define WSIZE       sizeof(void *)            /* word size 8 bytes */
#define DSIZE       (2 * WSIZE)            /* doubleword size 16 bytes */
#define MINCHUNKSIZE   (1 << 7)      /* min heap extension 128 bytes */
#define HF2PSIZE    (4 * WSIZE)   /* header, footer, 2 list pointers (successor, predecessor)*/
#define FREELISTLEN 112

#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

/* Pack a size and allocated bit into a word */
#define PACK_SIZE_ALLOC(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET_WORD(p)          (*(uintptr_t *)(p))
#define PUT_WORD(p,val)      (*(uintptr_t *)(p) = (val))

/* Read the size and allocated fields from address p (pointer to beginning of header) */
#define GET_SIZE_VAL(p)     (GET_WORD(p) & ~(DSIZE - 1))
#define GET_ALLOC_BIT(p)    (GET_WORD(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE_VAL(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE_VAL(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE_VAL(((char *)(bp) - DSIZE)))

/* get the closest next multiple of 16 of a number */
#define NEXT_MULT_16(num) ((num) % 16 == 0 ? (num) : (num) + (16 - (num) % 16))

/* free list methods */
#define LOG2(num) (63 - __builtin_clzll((num)))
#define POW2(n) ((unsigned long int) 1 << (n))
#define INDEXUPPERBOUND(index) ((index) <= 83 ? 32 + 16 * (index) : 1360 + 16 * POW2((index) - 83))

/* gotta save space for header and footer which are 16 bytes */
#define BIG_ENOUGH(block_size, user_request_size) ((block_size) >= ((user_request_size) + 16))

/* after block split, the remainder of the block should be able to hold header, footer, two pointers, and still have >0 payload*/
#define SHOULD_SPLIT(block_size, user_request_size) ((block_size) - ((user_request_size) + 16) > 16)

void *heap_listp = NULL;
static void *epilogue = NULL;
/* one block shouldn't need to exceed 2^32 bytes */

/****************************
 * singular numbers from index 0 - 83 (32 - 1360)
 * increment by powers of 2 from index 84 - 111 (1360 + 16 * 2^1 - 1360 + 16 * 2^28 )
****************************/
static char *free_list[FREELISTLEN];
/* block
|<--size + allocated bit-->|<--predecessor ptr-->|<--payload-->|<--successor ptr-->|<--size + allocated bit-->|
        8 bytes                 8 bytes                             8 bytes                 8 bytes
*/

/**********************************************************
 * void *mem sbrk(int incr): Expands the heap by incr bytes,
 * where incr is a positive non-zero integer and returns a generic pointer to the first byte of the newly allocated heap area. 
 * The semantics are identical to the Unix sbrk function, except that mem sbrk accepts only a positive non-zero integer argument.
 * void *mem_heap_lo(void): Returns a generic pointer to the first byte in the heap.
 * void *mem_heap_hi(void): Returns a generic pointer to the last byte in the heap.
 * size_t mem_heapsize(void): Returns the current size of the heap in bytes.
 * size_t mem_pagesize(void): Returns the system’s page size in bytes (4K on Linux systems).
*/

int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);
/* helper functions */
void *coalesce(void *bp, int only_next);
void add_block_to_free_list(void *bp);
void remove_block_from_free_list(void *bp);
void *extend_heap(size_t words);
void *find_fit(size_t asize);
void place(void* bp, size_t asize);
int mm_check(void);

/**********************************************************
 * mm_init
 * Initialize the heap, including "allocation" of the
 * prologue and epilogue
 **********************************************************/
 int mm_init(void)
 {
    for (int i = 0; i < FREELISTLEN; i++) {
        free_list[i] = NULL;
    }
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;

    PUT_WORD(heap_listp, 0);                         // alignment padding
    PUT_WORD(heap_listp + (1 * WSIZE), PACK_SIZE_ALLOC(DSIZE, 1));   // prologue header
    PUT_WORD(heap_listp + (2 * WSIZE), PACK_SIZE_ALLOC(DSIZE, 1));   // prologue footer
    PUT_WORD(heap_listp + (3 * WSIZE), PACK_SIZE_ALLOC(0, 1));    // epilogue header
    epilogue = heap_listp + (3 * WSIZE);
    heap_listp += DSIZE;

    return 0;
 }

/**********************************************************
 * coalesce
 * Covers the 4 cases discussed in the text:
 * - both neighbours are allocated
 * - the next block is available for coalescing
 * - the previous block is available for coalescing
 * - both neighbours are available for coalescing
 **********************************************************/
void *coalesce(void *bp, int only_next)
{
    bp = (char *) bp;
    size_t my_size = GET_SIZE_VAL(bp - 8);
    size_t prev_alloc = GET_ALLOC_BIT(bp - 16);

    size_t next_alloc = GET_ALLOC_BIT(bp - 8 + my_size);

    if (only_next && (!prev_alloc || next_alloc)) {
        return bp;
    }

    // printf("prev: %ld, next: %ld\n", prev_alloc, next_alloc);

    if (prev_alloc && next_alloc) {       /* Case 1 */
        // // assert(!from_extension && "Should not be case 1 coalesce after heap extension.");
        return (void *) bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
        // // assert(!from_extension && "Should not be case 2 coalesce after heap extension.");
        remove_block_from_free_list(bp - 8 + my_size + 8);
        void *my_header = bp - 8;
        size_t next_size = GET_SIZE_VAL((bp - 8) + my_size);
        /* bp - 8 + my_size should point to next header */
        void *next_footer = (bp - 8 + my_size) + next_size - 8;
        size_t total_size = my_size + next_size;
        PUT_WORD(my_header, PACK_SIZE_ALLOC(total_size, 0));
        PUT_WORD(next_footer, PACK_SIZE_ALLOC(total_size, 0));
        /* TODO: add logic for changing pointers when for free list */
        return (void *) (bp);
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
        size_t prev_size = GET_SIZE_VAL(bp - 16);
        void *prev_header = bp - 8 - prev_size;
        remove_block_from_free_list((char *) prev_header + 8);
        void *my_footer = bp - 8 + my_size - 8;
        
        size_t total_size = prev_size + my_size;
        PUT_WORD(prev_header, PACK_SIZE_ALLOC(total_size, 0));
        PUT_WORD(my_footer, PACK_SIZE_ALLOC(total_size, 0));
        prev_header = (char *) prev_header;
        /* TODO: add logic for changing pointers when for free list */
        return ((void *) (prev_header + 8));
    }

    else if (!prev_alloc && !next_alloc) {            /* Case 4, coalesce prev and next*/
        size_t prev_size = GET_SIZE_VAL(bp - 16);
        size_t next_size = GET_SIZE_VAL((bp - 8) + my_size);
        void *prev_header = bp - 8 - prev_size;
        remove_block_from_free_list((char *) prev_header + 8);
        remove_block_from_free_list((char *) bp - 8 + my_size + 8);
        void *next_footer = (bp - 8 + my_size) + next_size - 8;

        size_t total_size = prev_size + my_size + next_size;
        PUT_WORD(prev_header, PACK_SIZE_ALLOC(total_size, 0));
        PUT_WORD(next_footer, PACK_SIZE_ALLOC(total_size, 0));
        prev_header = (char *) prev_header;
        /* TODO: add logic for changing pointers when for free list */
        return (void *) (prev_header + 8);
    }
    // assert(0 && "Should not get here in coalesce.");
    return NULL;
}

/* based on block size, get the index of the free list the block belongs to */
size_t get_free_list_index(size_t block_size) {
    size_t index;
    if (block_size <= 1360) { 
        index = (block_size - 32) >> 4;
        return index;
    }
    else if (block_size > 1360) {
        index = 83 + LOG2(((block_size) - 1360) >> 4);
        size_t upper_bound = INDEXUPPERBOUND(index);
        if (block_size > upper_bound) {
            index++;
        }
    }
    // printf("index: %ld\n", index);
    // printf("size: %lu\n", block_size);
    // assert(index < FREELISTLEN && "get_free_list_index: index exceeds free list.");
    return index;
}

/* add block to front of free list */
void add_block_to_free_list(void *bp) {
    // printf("Adding block to free list!\n");
    bp = (char *) bp;
    size_t block_size = GET_SIZE_VAL(bp - 8);
    // assert(GET_ALLOC_BIT(bp - 8) == 0 && "Trying to add allocated block to free list.");
    // assert(block_size >= 32 && "Block to add to free list has size less than 32.");
    // assert(block_size % 16 == 0 && "Block to add to free list has size not multiple of 16.");
    // printf("Getting index from add.\n");
    size_t index = get_free_list_index(block_size);
    if (free_list[index] == NULL) {
        // printf("Adding block to index %ld that is NULL.\n", index);
        /* bp predecessor is NULL */
        PUT_WORD(bp, 0);
        /* bp successor is NULL */
        PUT_WORD(bp - 8 + block_size - 16, 0);
        free_list[index] = bp;
        // printf("free_list[index] ptr: 0x%x\n", free_list[index]);
        // printf("free_list[index] has alloc bit %ld\n", GET_ALLOC_BIT(free_list[index] - 8));
        // if (index == 16) {
        //     printf("free_list[7] has alloc bit %ld\n", GET_ALLOC_BIT(free_list[7] - 8));
        // }
        return;
    }
    else if (free_list[index] != NULL) {
        // printf("Adding block to index %ld that is NOT NULL.\n", index);
        /* predecessor of this block is NULL */
        PUT_WORD(bp, 0);
        size_t old_head_size = GET_SIZE_VAL(free_list[index] - 8);
        char *old_head_pred = free_list[index];
        char *old_head_succ = free_list[index] - 8 + old_head_size - 16;
        /* successor of this block is the successor of old head of the free list*/
        PUT_WORD(bp - 8 + block_size - 16, (size_t) old_head_succ);
        /* predecessor of old head is this new block */
        PUT_WORD(old_head_pred, (size_t) bp);
        free_list[index] = bp;
        // printf("free_list[index] has alloc bit %ld\n", GET_ALLOC_BIT(free_list[index] - 8));
        return;
    }
    // assert(0 && "should not get here in add block to free list.");
}

/****************************
 * remove a block from free list
 * This block must exist in the free list
 * case 1: block is only block in list (predecessor and successor NULL)
 * case 2: block is head (predecessor == NULL)
 * case 3: block is tail (successor == NULL)
 * case 4: block is in the middle (predecessor != NULL && successor != NULL)
*****************************/
void remove_block_from_free_list(void *bp) {
    // printf("Removing block from free list.\n");
    bp = (char *) bp;
    size_t block_size = GET_SIZE_VAL(bp - 8);
    // assert(GET_ALLOC_BIT(bp - 8) == 0 && "Block to remove from free list is allocated.");
    // assert(block_size >= 32 && "remove_block_from_free_list: block has size less than 32.");
    // assert(block_size % 16 == 0 && "remove_block_from_free-list: block has size not multiple of 16.");
    // printf("Getting index from remove.\n");
    size_t index = get_free_list_index(block_size);

    // assert(free_list[index] != NULL && "remove_block_from_free_list: NULL list head.");
    /* if the block is the only block in the list */
    if (GET_WORD((void *) bp) == 0 && GET_WORD((void *) (bp - 8 + block_size - 16)) == 0) {
        free_list[index] = NULL;
        return;
    }
    /* if the block is the head of the list */
    else if (GET_WORD((void *) bp) == 0) {
        // assert(free_list[index] == bp && "remove_block_from_free_list: bp is not the list head.");
        char *new_head_succ = (char *) GET_WORD((void *) (bp - 8 + block_size - 16));
        size_t new_head_size = GET_SIZE_VAL(new_head_succ + 8);
        // assert(new_head_size >= 32 && "remove_block_from_free_list: new_head_size < 32");
        // assert(new_head_size % 16 == 0 && "remove_block_from_free_list: new_head_size not multiple of 16");
        // assert(GET_ALLOC_BIT(new_head_succ + 8) == 0 && "remove_block_from_free_list: new head is allocated.");
        char *new_head_pred = new_head_succ + 16 - new_head_size + 8;

        /* set the succ of bp to NULL */
        PUT_WORD(bp - 8 + block_size - 16, 0);

        /* set predecessor of new head to NULL */
        PUT_WORD(new_head_pred, 0);
        free_list[index] = new_head_pred;
        return;
    }
    /* if the block is the tail of the list */
    else if (GET_WORD((void *) (bp - 8 + block_size - 16)) == 0) {
        char *new_tail_pred = (char *) GET_WORD((void *) bp);
        size_t new_tail_size = GET_SIZE_VAL(new_tail_pred - 8);
        // assert(new_tail_size >= 32 && "remove_block_from_free_list: new_tail_size < 32");
        // assert(new_tail_size % 16 == 0 && "remove_block_from_free_list: new_tail_size not multiple of 16");
        // assert(GET_ALLOC_BIT(new_tail_pred - 8) == 0 && "remove_block_from_free_list: new tail is allocated.");
        char *new_tail_succ = new_tail_pred - 8 + new_tail_size - 16;

        /* set predecessor of bp to NULL */
        PUT_WORD(bp, 0);

        /* set the successor of the new tail to NULL */
        PUT_WORD(new_tail_succ, 0);
        return;
    }
    /* if the block is in the middle of the list */
    else if (GET_WORD((void *) bp) != 0 && GET_WORD((void *) (bp - 8 + block_size - 16)) != 0) {
        char *prev_pred = (char *) GET_WORD((void *) bp);
        size_t prev_size = GET_SIZE_VAL(prev_pred - 8);
        // assert(prev_size >= 32 && "remove_block_from_free_list: previous size < 32");
        // assert(prev_size % 16 == 0 && "remove_block_from_free_list: previous size not multiple of 16");
        // assert(GET_ALLOC_BIT(prev_pred - 8) == 0 && "remove_block_from_free_list: previous block is allocated.");
        char *next_succ = (char *) GET_WORD((void *) (bp - 8 + block_size - 16));
        size_t next_size = GET_SIZE_VAL(next_succ + 8);
        // assert(next_size >= 32 && "remove_block_from_free_list: next size < 32");
        // assert(next_size % 16 == 0 && "remove_block_from_free_list: next size not multiple of 16");
        // assert(GET_ALLOC_BIT(next_succ + 8) == 0 && "remove_block_from_free_list: next block is allocated.");

        /* set predecessor and successor of bp to NULL */
        PUT_WORD(bp, 0);
        PUT_WORD(bp - 8 + block_size - 16, 0);

        char *prev_succ = prev_pred - 8 + prev_size - 16;
        char *next_pred = next_succ + 16 - next_size + 8;
        /* set the successor of the previous block to be the successor of the next block */
        PUT_WORD(prev_succ, (size_t) next_succ);
        /* set the predecessor of the next block to be the predecessor of the previous block */
        PUT_WORD(next_pred, (size_t) prev_pred);
        return;
    }
    // assert(0 && "Should not get here in remove block from free list.");
}

/**********************************************************
 * extend_heap
 * Extend the heap by "words" words, maintaining alignment
 * requirements of course. Free the former epilogue block
 * and reallocate its new header
 **********************************************************/
void *extend_heap(size_t words)
{
    // printf("Extending heap by %ld words.\n", words);
    char *bp;
    size_t size;

    size = words * WSIZE;
    // if (size % 16 != 0)

    /* align by 16 */
    size = NEXT_MULT_16(size);

    // save room for header and footer
    // note: extra 160 bytes allocation gives best throughput...
    size += 16 * 10;

    /* allocate at minimum min chunk size heap */
    // size = MAX(size, MINCHUNKSIZE);

    if ((bp = mem_sbrk(size)) == (void *) -1) {
        // assert(0 && "extend_heap error: mem_sbrk returned (void *) -1.");
        return NULL;
    }

    /* use old eplilogue to contain header */
    void *my_header = bp - 8;
    void *predecessor = bp;
    void *successor = bp + size - 24;
    void *my_footer = bp + size - 16;
    void *new_epilogue = bp + size - 8;
    /* pack header */
    PUT_WORD(my_header, PACK_SIZE_ALLOC(size, 0));             
    /* pointer to predecessor. TODO: add actual list ptr value */
    PUT_WORD(predecessor, 0);
    /* pointer to successor. TODO: add actual list ptr value */
    PUT_WORD(successor, 0);    
    /* pack footer */  
    PUT_WORD(my_footer, PACK_SIZE_ALLOC(size, 0));      
    /* pack epilogue */
    PUT_WORD(new_epilogue, PACK_SIZE_ALLOC(0, 1));

    /* free old epilogue, then coalesce old epilogue */
    epilogue = new_epilogue;
    // printf("Calling coalesce extend heap.\n");
    return coalesce(bp, 0);
}


/**********************************************************
 * find_fit
 * Traverse the heap searching for a block to fit asize
 * Return NULL if no free blocks can handle that size
 * Assumed that asize is aligned
 **********************************************************/
void *find_fit(size_t request_size)
{
    // printf("Getting index from find fit.\n");
    size_t start_index = get_free_list_index(request_size);
    size_t block_size;
    char *bp = NULL;
    char *successor;
    // assert(request_size % 16 == 0 && "find_fit: request size should be multiple of 16.");
    // char *bp;
    // assert(INDEXUPPERBOUND(start_index) >= request_size && "find_fit: size upper bound of index smaller than request.");
    // long int block_id = 0;
    for (; start_index < FREELISTLEN; start_index++) 
    {
        if (free_list[start_index] != NULL) {
            successor = free_list[start_index] - 8 + GET_SIZE_VAL(free_list[start_index] - 8) - 16;
            while (successor != 0) {
                block_size = GET_SIZE_VAL(successor + 8);
                if (BIG_ENOUGH(block_size, request_size)) {
                    bp = successor + 16 - block_size + 8;
                    break;
                }
                successor = (char *) GET_WORD(successor);
            }
            if (bp != NULL) {
                // printf("Found fit at index %ld and has size %lu\n", start_index, GET_SIZE_VAL(bp - 8));
                // printf("free_list[index] ptr: 0x%x\n", free_list[start_index]);
                // // assert(GET_ALLOC_BIT(free_list[start_index] - 8) == 0 && "Found block is allocated free list.");
                // // assert(GET_ALLOC_BIT(bp - 8) == 0 && "Found block is allocated.");
                remove_block_from_free_list(bp);
                return bp;
            }
        }
    }
    return NULL;
}


/**********************************************************
 * place
 * Mark the block as allocated
 **********************************************************/
void place(void *bp, size_t request_size)
{
    // assert(request_size % 16 == 0 && "place: request_size should be a multiple of 16.");
    bp = (char *) bp;
    /* Get the current block size */
    void *my_header = bp - 8;
    size_t block_size = GET_SIZE_VAL(my_header);
    void *my_footer = bp - 8 + block_size - 8;
    /* if we shouldn't split the block, use the whole block to allocate */
    if (!SHOULD_SPLIT(block_size, request_size)) {
        // put the size and allocated bytes to header and footer
        PUT_WORD(my_header, PACK_SIZE_ALLOC(block_size, 1));
        PUT_WORD(my_footer, PACK_SIZE_ALLOC(block_size, 1));
        return;
    }
    else if (SHOULD_SPLIT(block_size, request_size)) {
        size_t allocated_block_size = 16 + request_size;
        PUT_WORD(my_header, PACK_SIZE_ALLOC(allocated_block_size, 1));
        void *allocated_block_footer = bp + request_size;
        PUT_WORD(allocated_block_footer, PACK_SIZE_ALLOC(allocated_block_size, 1));
        size_t remaining_block_size = block_size - allocated_block_size;
        /* handle adding new header, changing footer value, adding new pointers*/
        void *remaining_block_header = bp + request_size + 8;
        PUT_WORD(remaining_block_header, PACK_SIZE_ALLOC(remaining_block_size, 0));
        PUT_WORD(my_footer, PACK_SIZE_ALLOC(remaining_block_size, 0));
        char *remaining_block = remaining_block_header + 8;
        // assert(GET_ALLOC_BIT(remaining_block - 8) == 0 && "Splitted remaining block has wrong alloc bit.");
        // printf("Calling add from place. Block has original size: %lu\n", block_size);
        // printf("Calling add from place. Trying to allocate size: %lu\n", request_size);
        // printf("Calling add from place. Block has size: %lu\n", GET_SIZE_VAL(remaining_block_header));
        add_block_to_free_list(remaining_block);
        /* TODO: Add new list pointers */
    }
}

/**********************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 **********************************************************/
void mm_free(void *bp)
{
    if(bp == NULL){
      return;
    }
    bp = (char *) bp;
    void *header = bp - 8;
    size_t size = GET_SIZE_VAL(header);
    void *footer = bp - 8 + size - 8;

    PUT_WORD(header, PACK_SIZE_ALLOC(size,0));
    PUT_WORD(footer, PACK_SIZE_ALLOC(size,0));
    // printf("Calling coalesce free.\n");
    char *free_block = coalesce(bp, 0);
    // assert(GET_ALLOC_BIT(free_block - 8) == 0 && "Coalesce returned allocated block.");
    // printf("Calling add from free.\n");
    add_block_to_free_list(free_block);
}


/**********************************************************
 * mm_malloc
 * Allocate a block of size bytes.
 * The type of search is determined by find_fit
 * The decision of splitting the block, or not is determined
 *   in place(..)
 * If no block satisfies the request, the heap is extended
 **********************************************************/
void *mm_malloc(size_t size)
{
    // printf("Calling malloc...");
    size_t request_size; /* adjusted block size */
    size_t extendsize; /* amount to extend heap if no fit */
    char * bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    request_size = NEXT_MULT_16(size);
    // assert(request_size >= size && "mm_malloc: request_size smaller than size");
    // assert(request_size % 16 == 0 && "mm_malloc: request_size not a multiple of 16");
    /* Search the free list for a fit */
    if ((bp = find_fit(request_size)) != NULL) {
        // printf("found fit\n");
        // remove_block_from_free_list(bp);
        place(bp, request_size);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = request_size;
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL) {
        // assert(0 && "Extend heap failed and returned NULL.");
        return NULL;
    }
    // printf("extended heap to find fit...\n");
    // printf("EXTENDED HEAP AND FOUND FIT!\n");
    place(bp, request_size);
    // printf("Finished....\n");
    // mm_check();
    return bp;

}

/**********************************************************
 * mm_realloc
 * Implemented simply in terms of mm_malloc and mm_free
 *********************************************************/
void *mm_realloc(void *ptr, size_t size)
{
    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0){
      mm_free(ptr);
      return NULL;
    }
    /* If oldptr is NULL, then this is just malloc. */
    if (ptr == NULL)
      return (mm_malloc(size));

    size = NEXT_MULT_16(size);
    size_t original_size = GET_SIZE_VAL(ptr - 8);
    if (size == original_size - 16) {
        return ptr;
    }
    else if (size < original_size - 16) {
        place(ptr, size);
        return ptr;
    }
    else if (size > original_size - 16) {
        PUT_WORD(ptr - 8, PACK_SIZE_ALLOC(original_size, 0));
        PUT_WORD(ptr - 8 + original_size - 8, PACK_SIZE_ALLOC(original_size, 0));
        void *new_ptr = coalesce(ptr, 1);
        size_t new_size = GET_SIZE_VAL(new_ptr - 8);
        if (BIG_ENOUGH(new_size, size)) {
            place(new_ptr, size);
            return new_ptr;
        }
        void *new_new_ptr = coalesce(new_ptr, 0);
        new_size = GET_SIZE_VAL(new_new_ptr - 8);
        if (BIG_ENOUGH(new_size, size)) {
            memcpy(new_new_ptr, new_ptr, original_size);
            place(new_new_ptr, size);
            return new_new_ptr;
        }

    }


    void *oldptr = ptr;
    void *newptr;
    size_t copySize = GET_SIZE_VAL(oldptr - 8);

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;

    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}

/**********************************************************
 * mm_check
 * Check the consistency of the memory heap
 * Return nonzero if the heap is consistant.
 *********************************************************/
int mm_check(void) {
    void *bp;
    unsigned long int block_id = 0;
    printf("Heap current size --> %ld bytes.\n", mem_heapsize());
    bp = heap_listp;
    int found = 0;
    while (bp <= mem_heap_hi())
    {
        if (GET_SIZE_VAL(bp) == 0) {
            printf("Epilogue found at block %ld\n", block_id);
            sleep(5);
            if (found) return 1;
            bp += 8;
            found = 1;
            // return 1;
            // break;
        }
        printf("Block ID: %ld ", block_id);
        printf("| Size of block: %ld |", GET_SIZE_VAL(HDRP(bp)));
        printf(" Block allocated: %ld |", GET_ALLOC_BIT(HDRP(bp)));
        printf("\n----------------------------------\n");
        block_id++;
        bp = NEXT_BLKP(bp);
    }
    return 1;
}

// void *best_fit(size_t asize) {
//     void *bp;
//     void *ans = NULL;
//     unsigned long long block_size;
//     unsigned long long min_size = 0xffffffffffffffff;
//     for (bp = heap_listp; GET_SIZE_VAL(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
//     {
//         block_size = GET_SIZE_VAL(HDRP(bp));
//         if (!GET_ALLOC_BIT(HDRP(bp)) && asize <= block_size && block_size < min_size)
//         {
//             min_size = block_size;
//             ans = bp;
//         }
//     }
//     return ans;
// }
