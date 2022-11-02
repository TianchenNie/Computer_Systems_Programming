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
#define EPISIZE     (2 * WSIZE)   /* make epilogue 16 byte aligned */
#define HF2PSIZE    (4 * WSIZE)   /* header, footer, 2 list pointers (successor, predecessor)*/

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

#define LOG2(num) (63 - __builtin_clzll((num)))

/* gotta save space for header and footer which are 16 bytes */
#define BIG_ENOUGH(block_size, user_request_size) ((block_size) >= ((user_request_size) + 16))

/* after block split, the remainder of the block should be able to hold header, footer, two pointers, and still have >0 payload*/
#define SHOULD_SPLIT(block_size, user_request_size) ((block_size) - ((user_request_size) + 16) > 16)

void *heap_listp = NULL;
static void *epilogue = NULL;

/* one block shouldn't need to exceed 2^64 bytes */
static uintptr_t *free_list[64];
static int first_time = 1;

/* 16, 32, 48, 64, 80, 96, 112, 128, 256, 512...*/

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
void *coalesce(void *bp);
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
    for (int i = 0; i < 64; i++) {
        free_list[i] = NULL;
    }
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
        return -1;

    first_time = 1;
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
 * if coalescing from heap extension, must be case 3 or case 4
 **********************************************************/
void *coalesce(void *bp)
{
    bp = (char *) bp;
    size_t my_size = GET_SIZE_VAL(bp - 8);
    size_t prev_alloc = GET_ALLOC_BIT(bp - 16);

    size_t next_alloc = GET_ALLOC_BIT(bp - 8 + my_size);

    // printf("prev: %ld, next: %ld\n", prev_alloc, next_alloc);

    if (prev_alloc && next_alloc) {       /* Case 1 */
        // assert(!from_extension && "Should not be case 1 coalesce after heap extension.");
        return (void *) bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
        // assert(!from_extension && "Should not be case 2 coalesce after heap extension.");
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
        void *next_footer = (bp - 8 + my_size) + next_size - 8;

        size_t total_size = prev_size + my_size + next_size;
        PUT_WORD(prev_header, PACK_SIZE_ALLOC(total_size, 0));
        PUT_WORD(next_footer, PACK_SIZE_ALLOC(total_size, 0));
        prev_header = (char *) prev_header;
        /* TODO: add logic for changing pointers when for free list */
        return (void *) (prev_header + 8);
    }
    assert(0 && "Should not get here in coalesce.");
    return NULL;
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
    size += 16;

    /* allocate at minimum min chunk size heap */
    size = MAX(size, MINCHUNKSIZE);

    if ((bp = mem_sbrk(size)) == (void *) -1) {
        assert(0 && "extend_heap error: mem_sbrk returned (void *) -1.");
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
    return coalesce(bp);
}


/**********************************************************
 * find_fit
 * Traverse the heap searching for a block to fit asize
 * Return NULL if no free blocks can handle that size
 * Assumed that asize is aligned
 **********************************************************/
void *find_fit(size_t request_size)
{
    assert(request_size % 16 == 0 && "find_fit: request size should be multiple of 16.");
    char *bp;
    // long int block_id = 0;
    for (bp = heap_listp; GET_SIZE_VAL(bp - 8) > 0; bp += GET_SIZE_VAL(bp - 8))
    {
        // if (GET_SIZE_VAL(bp) == 0) {
        //     printf("Epilogue found at block %ld\n", block_id);
        //     // bp += 8;
        //     // continue;
        //     // break;
        // }
        // printf("Block ID: %ld ", block_id);
        // printf("| Size of block: %ld |", GET_SIZE_VAL(HDRP(bp)));
        // printf(" Block allocated: %ld |", GET_ALLOC_BIT(HDRP(bp)));
        // printf("\n----------------------------------\n");
        // block_id++;
        // bp = NEXT_BLKP(bp);
        size_t block_size = GET_SIZE_VAL(bp - 8);
        if (!GET_ALLOC_BIT(bp - 8) && BIG_ENOUGH(block_size, request_size))
        {
            return bp;
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
    assert(request_size % 16 == 0 && "place: request_size should be a multiple of 16.");
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
    coalesce(bp);
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
    size_t request_size; /* adjusted block size */
    size_t extendsize; /* amount to extend heap if no fit */
    char * bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    request_size = NEXT_MULT_16(size);
    assert(request_size >= size && "mm_malloc: request_size smaller than size");
    assert(request_size % 16 == 0 && "mm_malloc: request_size not a multiple of 16");
    /* Search the free list for a fit */
    if ((bp = find_fit(request_size)) != NULL) {
        // printf("FOUND FIT!\n");
        place(bp, request_size);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = request_size;
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    // printf("EXTENDED HEAP AND FOUND FIT!\n");
    place(bp, request_size);
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

    void *oldptr = ptr;
    void *newptr;
    size_t copySize;

    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;

    /* Copy the old data. */
    copySize = GET_SIZE_VAL(oldptr - 8);
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
