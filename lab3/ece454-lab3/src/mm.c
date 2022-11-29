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
    "bianbian",
    /* First member's first name and last name */
    "Sharon:Li",
    /* First member's student number */
    "1004947800",
    /* Second member's first name and last name (do not modify if working alone) */
    "",
    /* Second member's student number (do not modify if working alone) */
    ""
};

/*************************************************************************
 * Basic Constants and Macros
 * You are not required to use these macros but may find them helpful.
*************************************************************************/
#define WSIZE       sizeof(void *)            /* word size (bytes) */
#define DSIZE       (2 * WSIZE)            /* doubleword size (bytes) */
#define CHUNKSIZE   (1<<7)      /* initial heap size (bytes) */

#define MAX(x,y) ((x) > (y)?(x) :(y))

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)          (*(uintptr_t *)(p))
#define PUT(p,val)      (*(uintptr_t *)(p) = (val))

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)     (GET(p) & ~(DSIZE - 1))
#define GET_ALLOC(p)    (GET(p) & 0x1)

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)        ((char *)(bp) - WSIZE)
#define FTRP(bp)        ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

#define LISTSIZE 32
void* heap_listp = NULL;

typedef struct freeBlock {
    struct freeBlock* next;
    struct freeBlock* prev;
} freeBlock;

freeBlock* freeList[LISTSIZE];
/**********************************************************
 * findListLocation
 * return the index of the list that the block belongs to in the freeList 
 * e.g. for asize of 127, it belongs to the block of 64~128, 
 * the block 64~128 has the index of 3, so 3 should be returned  
 **********************************************************/
int findLocationInFreeList (size_t size)
{
    printf("FIND LOCATION IN FREE LIST SIZE: %lu\n", size);
    printf("CURR HEAP SIZE: %lu\n", mem_heapsize());
    int count = 0;
    while (size != 0 ) {
        size = size >> 1;
        count ++;
    }

    if (count - 5 < 0) {
        return 0;
    } else {
        return count - 5;
    }

    // count = max(count - 5, 0);
    if (count > LISTSIZE) {
        return LISTSIZE - 1;
    } else {
        return count;
    }
}
/**********************************************************
 * place
 * Mark the block as allocated
 **********************************************************/
void place(void* bp, size_t asize)
{
  /* Get the current block size */
//   size_t bsize = GET_SIZE(HDRP(bp));
  printf("PLACE SIZE: %lu", asize);
  PUT(HDRP(bp), PACK(asize, 1));
  PUT(FTRP(bp), PACK(asize, 1));
}

/**********************************************************
 * unplace
 * Mark the block as free
 **********************************************************/
void unplace(void* bp, size_t asize)
{
  /* Get the current block size */
//   size_t bsize = GET_SIZE(HDRP(bp));

  PUT(HDRP(bp), PACK(asize, 0));
  PUT(FTRP(bp), PACK(asize, 0));
}

/**********************************************************
 * removeFromFreeList
 **********************************************************/
void removeFromFreeList (freeBlock* block) 
{
    if (block == NULL) {
        return;
    }
    void * bp = (void *) block;
    size_t size = GET_SIZE(HDRP(block));
    assert(size >= 16);
    int index = findLocationInFreeList(GET_SIZE(HDRP(block)));
    // printf("RFFL INDEX: %d\n", index);
    assert(freeList[index] != NULL);
    if (block -> prev == NULL) { //at the beginning of list
        freeList[index] = block -> next;
        if (block->next != NULL) {
            (block -> next) -> prev = NULL;
            size_t size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
            // printf("RFFL SIZE: %ld\n", size);
            assert(size >= 16);
        }
        return;
    } else if (block -> next == NULL) { //at the end of list
        if (block -> prev != NULL) 
            (block -> prev) -> next = NULL;
        return;
    } else if (block -> next != NULL && block -> prev != NULL) { //middle of list
        size_t size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        // printf("RFFL SIZE: %ld\n", size);
        assert(size >= 16);
        // printf("Block next: 0x%x\n", block -> next);
        (block -> prev) -> next = block -> next;
        (block -> next) -> prev = block -> prev;
        return;
    }
    assert(0);
}
/**********************************************************
 * addToFreeList
 **********************************************************/
void addToFreeList(freeBlock* block) {
    if (block == NULL) {
        return;
    }

    size_t size = GET_SIZE(HDRP(block));
    // printf("ADDING TO FREE LIST %ld\n", size);
    // printf("ATFL SIZE: %ld\n", size);
    assert(size >= 16);
    //size_t size = findLocationInFreeList(HDRP(block)) - DSIZE;
    int index = findLocationInFreeList(GET_SIZE(HDRP(block)));
    
    if (freeList[index] == NULL) { //if the list is empty
        // printf("FREE LIST EMPTY\n");
        freeList[index] = block;
        block -> next = NULL;
        block -> prev = NULL;
        // size_t size = GET_SIZE(HDRP(block));
        // printf("ATFL SIZE: %ld\n", size);
    } else {
        block -> next = freeList[index];
        block -> prev = NULL;
        freeList[index] -> prev = block;
        freeList[index] = block;
        size_t size = GET_SIZE(HDRP(block));
        // printf("ATFL SIZE: %ld\n", size);
        size = GET_SIZE(HDRP(NEXT_BLKP(block)));
        // printf("ATFL NEXT SIZE: %ld\n", size);
        assert(size >= 16);
    }
    return;
}

/**********************************************************
 * split
 * when given block is larger than the size of need, split
 * 
 **********************************************************/
void* split(void *bp, size_t asize)
{   
    
    size_t block_size = GET_SIZE(HDRP(bp));
    assert(block_size >= asize && "split");

    //if free_block_size is bigger than the size user needs, then split the block
    if (block_size - asize >= (16 * 2)) {
        char *footer = FTRP(bp);
        //header for asize block, users asked for the asize block
        PUT(HDRP(bp), PACK(asize,1));
        //footer for asize block, user asked for the asize block
        PUT(((char *)bp + asize - DSIZE), PACK(asize,1));
        //header for blocksize - asize block
        PUT(((char *)bp + asize - WSIZE), PACK(block_size - asize,0));
        //footer for the free block after split
        PUT(footer, PACK((block_size - asize),0));
        addToFreeList((freeBlock *) ((char *)bp + asize));
    } else if (block_size == asize) {
        return bp;
    }
    return bp;
}

/**********************************************************
 * mm_init
 * Initialize the heap, including "allocation" of the
 * prologue and epilogue
 **********************************************************/
 int mm_init(void) 
 {
    // printf("Size of free block: %d", sizeof(freeBlock));  
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)
         return -1;
     PUT((char *) heap_listp, 0);                         // alignment padding
     PUT((char *)heap_listp + (1 * WSIZE), PACK(DSIZE, 1));   // prologue header
     PUT((char *)heap_listp + (2 * WSIZE), PACK(DSIZE, 1));   // prologue footer
     PUT((char *)heap_listp + (3 * WSIZE), PACK(0, 1));    // epilogue header
     heap_listp = (char *) heap_listp + DSIZE;

     for (int i = 0; i < LISTSIZE; i++) {
        freeList[i] = NULL;
     }

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
void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {       /* Case 1 */
        return bp;
    }

    else if (prev_alloc && !next_alloc) { /* Case 2 */
        removeFromFreeList((freeBlock *) NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        return (bp);
    }

    else if (!prev_alloc && next_alloc) { /* Case 3 */
        removeFromFreeList((freeBlock *) PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        return (PREV_BLKP(bp));
    }

    else {            /* Case 4 */
        removeFromFreeList((freeBlock *) PREV_BLKP(bp));
        size_t size = GET_SIZE(HDRP(NEXT_BLKP(bp)));
        size_t alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
        // printf("SIZE: %ld\n", size);
        // printf("ALLOC: %ld\n", alloc);
        removeFromFreeList((freeBlock *) NEXT_BLKP(bp));

        size += GET_SIZE(HDRP(PREV_BLKP(bp)))  +
            GET_SIZE(FTRP(NEXT_BLKP(bp)))  ;
        PUT(HDRP(PREV_BLKP(bp)), PACK(size,0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size,0));
        return (PREV_BLKP(bp));
    }
    assert(0);
}

/**********************************************************
 * extend_heap
 * Extend the heap by "words" words, maintaining alignment
 * requirements of course. Free the former epilogue block
 * and reallocate its new header
 **********************************************************/
void *extend_heap(size_t words)
{
    char *bp;
    size_t size;

    /* Allocate an even number of words to maintain alignments */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ( (bp = mem_sbrk(size)) == (void *)-1 )
        return NULL;

    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));                // free block header
    PUT(FTRP(bp), PACK(size, 0));                // free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));        // new epilogue header

    /* Coalesce if the previous block was free */
    return coalesce(bp);
}


/**********************************************************
 * find_fit
 * Traverse the heap searching for a block to fit asize
 * Return NULL if no free blocks can handle that size
 * Assumed that asize is aligned
 **********************************************************/
void * find_fit(size_t asize)
{
    // void *bp;
    int listIndex = findLocationInFreeList(asize);

    for (int i = listIndex; i < LISTSIZE; i++) {
        //loop through list, start from the minimum sized list that can fit this block
        if (freeList[i] != NULL) { 
            freeBlock* curr = freeList[i];
            while (curr != NULL) {
                size_t currSize = GET_SIZE(HDRP(curr));
                // printf("CURR SIZE: %ld\n", currSize);
                if (currSize >= asize) {
                    // printf("CURR SIZE: %d\n", currSize);
                    removeFromFreeList(curr);
                    // curr = split(curr, asize);
                    return (void*) curr;
                }
                curr = curr -> next;
            }
        }
    }
    // for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp))
    // {
    //     if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp))))
    //     {
    //         return split(bp, asize);
    //     }
    // }
    return NULL;
}

/**********************************************************
 * mm_free
 * Free the block and coalesce with neighbouring blocks
 **********************************************************/
void mm_free(void *bp)
{
    if (bp == NULL) {
      return;
    }
    assert(GET_ALLOC(HDRP(bp)) == 1);

    size_t size = GET_SIZE(HDRP(bp));
    printf("SIZE BEFORE FREE: %ld\n", size);
    PUT(HDRP(bp), PACK(size,0));
    PUT(FTRP(bp), PACK(size,0));
    bp = coalesce(bp);
    size = GET_SIZE(HDRP(bp));
    printf("SIZE AFTER FREE: %ld\n", size);
    addToFreeList((freeBlock*) bp);
    return;
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
    size_t asize; /* adjusted block size */
    size_t extendsize; /* amount to extend heap if no fit */
    char * bp;

    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)
        asize = 2 * DSIZE;
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1))/ DSIZE);
    
    /* Search the free list for a fit */
    if ((bp = find_fit(asize)) != NULL) {
        // bp = split(bp, asize);
        size_t returnSize = GET_SIZE(HDRP(bp));
        place(bp, returnSize);
        return bp;
    }

    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize, CHUNKSIZE);
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    // bp = split(bp, asize);
    size_t returnSize = GET_SIZE(HDRP(bp));
    place(bp, returnSize);
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
    copySize = GET_SIZE(HDRP(oldptr));
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
int mm_check(void){
  return 1;
}
