// mm-naive.c - The fastest, least memory-efficient malloc package.
//
// In this naive approach, a block is allocated by simply incrementing
// the brk pointer.  A block is pure payload. There are no headers or
// footers.  Blocks are never coalesced or reused. Realloc is
// implemented directly using mm_malloc and mm_free.
//
// NOTE TO STUDENTS: Replace this header comment with your own header
// comment that gives a high level description of your solution.
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

static size_t find_sizeclass(size_t size);

// NOTE TO STUDENTS: Before you do anything else, please
// provide your team information in the following struct.
team_t team = {
    // Team name
    "ateam",
    // First member's full name
    "Tommy Yu",
    // First member's email address
    "bovik@cs.cmu.edu",
    // Second member's full name (leave blank if none)
    "",
    // Second member's email address (leave blank if none)
    ""
};

#define ALIGNMENT 8
#define MINBLOCKSIZE 24

// rounds up to the nearest multiple of ALIGNMENT
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

// Basic constants and macros
#define HDR 4
#define FTR 4
#define WSIZE 4 // Word and header/footer size (bytes)
#define DSIZE 8 // Double word size (bytes)
#define CHUNKSIZE (1<<12) // Extend heap by this amount (bytes)
#define MAX(x, y) ((x) > (y)? (x) : (y))

// Pack a size and allocated bit into a word
#define PACK(size, prealloc, alloc) ((size) | (prealloc << 1) | (alloc))
#define GET(p) (*(unsigned int *) (p))
#define PUT(p, val) (*(unsigned int *) (p) = (val))
#define PUT_PTR(p, ptr) (*(char **)(p) = (char *)(ptr))

// set state bits, take header word
#define SET_ALLOC(p) (GET(p) |= 0x1)
#define CLR_ALLOC(p) (GET(p) &= ~0x1)
#define SET_PRE_ALLOC(p) (GET(p) |= 0x2)
#define CLR_PRE_ALLOC(p) (GET(p) &= ~0x2)

// Read the size and allocated fields from address p (usually header/footer)
#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_PRE_ALLOC(p) ((GET(p) & 0x2) >> 1)

#define PRE_IS_ALLOC(bp) (GET_PRE_ALLOC(HDRP(bp)))
#define NEXT_IS_ALLOC(bp) (GET_ALLOC(HDRP(NEXT_BLKP(bp))))

// Given block ptr bp, compute address of its header and footer
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

// Given block ptr bp, compute address of next and previous blocks
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))

// Returns address of block, land at the first block 
#define HEADLIST(index) *((char **)((char *)heap_listp + (index * DSIZE)))
// Global variables
static char *heap_listp = NULL;   // pointer to data structure | free list heads | Prolouge header | ...

// mm_init - initialize the malloc package.
int mm_init(void) {
    // Create the initial empty heap
    if ((heap_listp = mem_sbrk(22*WSIZE)) == (void *)-1) { return -1; }
    
    // Size class heads
    PUT_PTR(heap_listp + (0*WSIZE), NULL);            // [24-31]
    PUT_PTR(heap_listp + (2*WSIZE), NULL);            // [32-63]
    PUT_PTR(heap_listp + (4*WSIZE), NULL);            // [64-127]
    PUT_PTR(heap_listp + (6*WSIZE), NULL);            // [128-255]
    PUT_PTR(heap_listp + (8*WSIZE), NULL);            // [256-511]
    PUT_PTR(heap_listp + (10*WSIZE), NULL);           // [512-1023]
    PUT_PTR(heap_listp + (12*WSIZE), NULL);           // [1024-2047]
    PUT_PTR(heap_listp + (14*WSIZE), NULL);           // [2048-4095]
    PUT_PTR(heap_listp + (16*WSIZE), NULL);           // [4096-inf]  
    PUT(heap_listp + (18*WSIZE), 0);              // Alignment padding
    PUT(heap_listp + (19*WSIZE), PACK(DSIZE, 1, 1)); // Prologue header
    PUT(heap_listp + (20*WSIZE), PACK(DSIZE, 1, 1)); // Prologue footer
    PUT(heap_listp + (21*WSIZE), PACK(0, 0, 1));     // Epilogue header
    

    return 0;
}

//return beginning of new block
static void *extend_heap(size_t words) {
    char *bp;
    size_t size;

    // Allocate an even number of words to maintain alignment
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;

    // Initialize free block header/footer, prev/succ pointer and the epilogue header

    //get pre state from old epilouge
    int prev_alloc = GET_PRE_ALLOC(HDRP(bp));

    PUT(HDRP(bp), PACK(size, prev_alloc, 0));         // Free block header
    PUT((bp), NULL);                      // Prev
    PUT((bp+DSIZE), NULL);                // Next
    PUT(FTRP(bp), PACK(size, prev_alloc, 0));         // Free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 0, 1)); // New epilogue header

    // Coalesce if the previous block was free
    return coalesce(bp);
}

// mm_malloc - Allocate a block by incrementing the brk pointer.
//     Always allocate a block whose size is a multiple of the alignment.
void *mm_malloc(size_t size) {
    size_t asize;      // Adjusted block size
    size_t extendsize; // Amount to extend heap if no fit
    char *bp;
    if (size == 0) { return NULL; } // Ignore spurious requests

    //Determine size class 
    //First or best fit seach free list
    //Split and insert fragments in appropraite list
    //if cant find search next class size
    //if cant find allocate from heap and split remaining in appropriate list

    if (size <= MINBLOCKSIZE - HDR) // Adjust block size to include overhead and alignment reqs.
        asize = MINBLOCKSIZE;
    else {
        // at least allocate 4 + size allgined
        size_t min = size + HDR;
        asize = DSIZE * ((min + (DSIZE-1)) / DSIZE); // Round to align
    }

    //Determin the size class
    size_t class_index = find_sizeclass(asize);

    // First or best fit search freelist
    // go to the free list and find the free block 

    bp = find_block(class_index, asize);



    //old code
    if ((bp = find_fit(asize)) != NULL) { // Search the free list for a fit
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE); // No fit found. Get more memory and place the block
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
        return NULL;
    place(bp, asize);
    return bp;
}


// find free block
// returns pointer to the block 
// maybe do some state updates?
static void* find_block(size_t class_index, size_t blocksize) {
    char *bp;
    bp = HEADLIST(class_index); //lands at first block

    while ((*(bp)) == NULL) {          //while NULL
        if (class_index < 8) {  //not last size
            bp = HEADLIST(class_index + 1); //increment class index
        } else { // all class NULL, allocate block and return
            size_t extend_size = MAX(blocksize, CHUNKSIZE);
            if ((bp = extend_heap(extend_size/WSIZE)) == NULL) {
                return NULL; //error handle?
            } else { //no error
                return bp;
            }
        }
    }
    // iterate inside one class to find the free block
    while (!((!(GET_ALLOC(HDRP(bp)))) && (GET_SIZE(HDRP(bp)) >= blocksize))) { // == needs to change?
        //go next 
        bp = *((char *)bp + DSIZE); //bp=bp->next
        if (bp == NULL) { // what if we reach end?
            //go next size class 
            if (class_index < 8) {  //not last size
            bp = HEADLIST(class_index + 1); //increment class index
            } else { // all class exhausted, allocate block and return
                size_t extend_size = MAX(blocksize, CHUNKSIZE);
                if ((bp = extend_heap(extend_size/WSIZE)) == NULL) {
                    return NULL; //error handle?
                } else { //no error
                    return bp;
                }
            }
        }
        
    } 
    // found 
    // split
    bp = split(bp, blocksize);

    // update curr and next bits
    SET_ALLOC(HDRP(bp));
    // got to next block and set next's prev
    SET_PRE_ALLOC(HDRP(NEXT_BLKP(bp)));
    // relink data structure
    //bp->prev->next = bp->next;
    PUT_PTR(((*bp) + DSIZE), *(bp + DSIZE));
    //bp->next->prev = bp->prev;
    PUT_PTR((*((bp) + DSIZE)), (*bp));
    //bp->prev = null; // is this optional? leave as garbage
    PUT_PTR(bp, NULL);
    //bp->next = null;
    PUT_PTR((bp + DSIZE), NULL);
    // clear garbage pointers 
    // return
    return bp; //bp read
}

// split extra block and place fragment in appropriate size class
// return useable partion of block
static void* split(char *bp, size_t blocksize) {
    // check if it needs splitting
    size_t size = GET_SIZE(HRDP(bp));
    size_t diff = size - blocksize; // unsigned problem 
    if (!(diff < MINBLOCKSIZE)) {
        // split 
        bp = split(bp, blocksize);
    }
    if (blocksize < 24)
}


// #define HEADLIST(index) *((char **)((char *)heap_listp + (index * DSIZE)))

// helper to find size class
// returns index of head list
static size_t find_sizeclass(size_t size) {
    /* maybe use bitwise for optimization  */ 
    if (24 <= size && size < 32) { 
        return 0;
    } else if (32 <= size && size < 64) {
        return 1;
    } else if (64 <= size && size < 128) {
        return 2;
    } else if (128 <= size && size < 256) {
        return 3;
    } else if (256 <= size && size < 512) {
        return 4;
    } else if (512 <= size && size < 1024) {
        return 5;
    } else if (1024 <= size && size < 2048) {
        return 6;
    } else if (2048 <= size && size < 4096) {
        return 7;
    } else if (4096 <= size) {
        return 8;
    }
}

// mm_free - Freeing a block does nothing.
void mm_free(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

//returns bp
static void *coalesce(void *bp) {
    // Allocated = No footer
    // Not allocated = Footer
    size_t prev_alloc = PRE_IS_ALLOC(bp);
    size_t next_alloc = NEXT_IS_ALLOC(bp);
    size_t size = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            // Case 1
        return bp;
    }

    else if (prev_alloc && !next_alloc) {      // Case 2 // Next free
        // Relink double linked list USE PUT_PTR
        PUT((*NEXT_BLKP(bp)), bp);
        PUT((bp), *NEXT_BLKP(bp));
        PUT((bp+DSIZE), *(NEXT_BLKP(bp) + DSIZE));
        PUT(*(NEXT_BLKP(bp) + DSIZE), bp);

        // Clear old pointer
        PUT(NEXT_BLKP(bp), NULL);
        PUT((NEXT_BLKP(bp) + DSIZE), NULL);

        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    }

    else if (!prev_alloc && next_alloc) {      // Case 3 // prev free is unchanged
        //verified
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        int succ_prev_alloc = GET_PRE_ALLOC((HDRP(PREV_BLKP(bp)))); // Get succ states
        PUT(FTRP(bp), PACK(size, succ_prev_alloc, 0));              // new size + old state
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, succ_prev_alloc, 0));
        bp = PREV_BLKP(bp);
    }

    else {                                     // Case 4 // Both side free
        // Relink double linked list USE PUT_PTR
        PUT(bp + DSIZE, NULL);
        PUT(bp, NULL);

        PUT((PREV_BLKP(bp) + DSIZE), *(NEXT_BLKP(bp) + DSIZE));
        PUT(*(NEXT_BLKP(bp) + DSIZE) , PREV_BLKP(bp));
        
        PUT(NEXT_BLKP(bp) + DSIZE, NULL);
        PUT(NEXT_BLKP(bp), NULL);

        size += GET_SIZE(HDRP(PREV_BLKP(bp))) +
                GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));

        // Bp still in middle



        //bp = PREV_BLKP(bp);
    }
    return bp;
}










// mm_realloc - Implemented simply in terms of mm_malloc and mm_free
void *mm_realloc(void *ptr, size_t size) {
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = *(size_t *)((char *)oldptr - SIZE_T_SIZE);
    if (size < copySize)
      copySize = size;
    memcpy(newptr, oldptr, copySize);
    mm_free(oldptr);
    return newptr;
}














