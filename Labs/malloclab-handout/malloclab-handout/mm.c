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

static void *extend_heap(size_t words);
static void* find_block(size_t class_index, size_t asize);
static void split(char *bp, size_t asize);
static void list_insert(char *bp);
static void list_remove(char *bp);
static size_t find_sizeclass(size_t size);
static void *coalesce(void *bp);
static void mm_checkheap(int lineno);
static int loop_detection(char *bp);

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

#define checkheap(lineno) mm_checkheap(lineno)
//#define checkheap(lineno)

#define NCLASSES 9

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

// Given a block ptr bp, access prev or next linked free block
#define PREV(bp) (*((char **)bp))
#define NEXT(bp) (*((char **)(((char *)(bp)) + DSIZE)))


// Returns address of block, land at the first block 
#define HEADLIST(index) (*((char **)((char *)heap_listp + (index * DSIZE))))

// Global variables
static char *heap_listp = NULL;   // pointer to data structure | free list heads | Prolouge header | ...
static char *prologue_bp;

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
    PUT(heap_listp + (21*WSIZE), PACK(0, 1, 1));     // Epilogue header. prev cont. block allocated

    size_t heads = NCLASSES * DSIZE;
    size_t pad = (heads + WSIZE) % DSIZE ? WSIZE : 0;  // pad so prologue payload is 8-aligned
    prologue_bp = heap_listp + heads + pad;
    checkheap(__LINE__);
    return 0;
}

//return beginning of new block or NULL sbrk failed
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

    PUT(HDRP(bp), PACK(size, prev_alloc, 0));         // Free block's new header
    PUT(FTRP(bp), PACK(size, prev_alloc, 0));         // Free block footer
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 0, 1)); // New epilogue header. Prev cont. block just allocated is free
    
    // Coalesce runs case 1 or 3
    return coalesce(bp);
}

// mm_malloc - Allocate a block by incrementing the brk pointer.
//     Always allocate a block whose size is a multiple of the alignment.
void *mm_malloc(size_t size) {
    size_t asize;      // Adjusted block size
    char *bp;
    if (size == 0) { return NULL; } // Ignore spurious requests
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

    // return null if can't find block 
    if ((bp = find_block(class_index, asize)) == NULL) { // if cant find block, extend
        size_t extend_size = MAX(asize, CHUNKSIZE);
        if ((bp = extend_heap(extend_size/WSIZE)) == NULL) {
            return NULL;
        }
    }
    
    // call before split
    // split changes size which list remove depends on to find size class
    list_remove(bp);

    // test for split
    size_t csize = GET_SIZE(HDRP(bp)); // block size of 
    size_t diff = csize - asize; // size always >= asize
    if ((diff >= MINBLOCKSIZE)) {
        // split. Next contiguous block = fragment itself
        split(bp, asize);
    } else {
        // set next block's prev bits. Next contiguous block = next contiguous block
        SET_PRE_ALLOC(HDRP(NEXT_BLKP(bp)));
    }

    // update curr bits
    SET_ALLOC(HDRP(bp));

    return bp;
}

// find free block
// returns pointer to the block 
// maybe do some state updates? -no, only find block 
static void* find_block(size_t class_index, size_t asize) {
    for(;class_index < NCLASSES; class_index++) {
        for (char *bp = HEADLIST(class_index); bp; bp = NEXT(bp)) { //iterate blocks
            if (GET_SIZE(HDRP(bp)) >= asize && !GET_ALLOC(HDRP(bp))) {
                return bp; // found
            }
        }    
    }
    return NULL;
    // this logic took longer than it should to write
}

// split extra block and place fragment in appropriate size class
static void split(char *bp, size_t asize) {
    // split any surplus of 24+ 
    char *frag_bp = bp + asize;
    size_t csize = GET_SIZE(HDRP(bp));
    size_t frag_size = csize - asize;
   
    // Pack headers/footers
    // Being allocated block - no footer
    PUT(HDRP(bp), PACK(asize, GET_PRE_ALLOC(HDRP(bp)), 1));
    //PUT(FTRP(bp), PACK(asize, GET_PRE_ALLOC(HDRP(bp)), 1));

    // Free block / set next block's prev bits
    PUT(HDRP(frag_bp), PACK(frag_size, 1, 0)); 
    PUT(FTRP(frag_bp), PACK(frag_size, 1, 0));

    // Insert frag to new list
    list_insert(frag_bp);
}

// Insert policy for free block
// Find and inserts free block into data structure
static void list_insert(char *bp) { // LIFO
    // take a free block
    // insert at beginning
    size_t index = find_sizeclass(GET_SIZE(HDRP(bp)));
    char *first = HEADLIST(index); //lands first block

    HEADLIST(index) = bp; // LIFO 
    PREV(bp) = NULL;

    if (first == NULL) { // If size class was empty 
        NEXT(bp) = NULL; // bp->Next = 0;
    } else {
        NEXT(bp) = first;
        PREV(first) = bp;
    }
}

// remove a block from data structure, relink
// remove addr pointing to this block 
static void list_remove(char *bp) {
    char *prev_p = PREV(bp); // land on block bp or Null
    char *next_p = NEXT(bp); // land on block bp or Null
    size_t index = find_sizeclass(GET_SIZE(HDRP(bp)));
    // Case 1 prev null, next null
    if (!prev_p && !next_p) {
        HEADLIST(index) = NULL; // head->next null
    }
    // Case 2 prev 1, next null
    else if (prev_p && !next_p) {
        NEXT(PREV(bp)) = NULL; //bp->prev->next = Null;
    }
    // Case 3 prev null, next 1;
    else if (!prev_p && next_p) {
         // head->next = bp->next;
        HEADLIST(index) = NEXT(bp);
        // bp->next->prev = NULL;
        PREV(NEXT(bp)) = NULL;
    }
    // Case 4 prev 1, next 1
    else if (prev_p && next_p) {
        // bp->prev->next = bp->next;
        NEXT(PREV(bp)) = NEXT(bp);
        // bp->next->prev = bp->prev
        PREV(NEXT(bp)) = PREV(bp);
    }
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
    return -1;
}

// mm_free - Freeing a block does nothing.
void mm_free(void *bp) {
    size_t csize = GET_SIZE(HDRP(bp));
    //change allocation bit
    //give self footer
    CLR_ALLOC(HDRP(bp));
    PUT(HDRP(bp), PACK(csize, GET_PRE_ALLOC(HDRP(bp)), 0));
    PUT(FTRP(bp), PACK(csize, GET_PRE_ALLOC(HDRP(bp)), 0));
    //coalesce
    coalesce(bp);
}

//returns bp
static void *coalesce(void *bp) {
    // Allocated = No footer
    // Not allocated = Footer
    size_t prev_alloc = PRE_IS_ALLOC(bp);
    size_t next_alloc = NEXT_IS_ALLOC(bp);
    size_t csize = GET_SIZE(HDRP(bp));

    if (prev_alloc && next_alloc) {            // Case 1
        // bp just freed, not in free list
        // notifiy next alloc block, prev is free
        SET_PRE_ALLOC(HDRP(NEXT_BLKP(bp)));

        // insert coalesce block into new size class
        list_insert(bp);
    }

    else if (prev_alloc && !next_alloc) {      // Case 2 // Next free
        // bp just freed, not in free list
        // remove next contiguous block from its free list
        char *n_bp = (NEXT_BLKP(bp));
        list_remove(n_bp);

        // coalsece block - Header, size allocate states
        size_t tsize = csize + GET_SIZE(HDRP(n_bp));  // total size
        PUT(HDRP(bp), PACK(tsize, GET_PRE_ALLOC(HDRP(bp)), 0));
        PUT(FTRP(bp), PACK(tsize, GET_PRE_ALLOC(HDRP(bp)), 0));

        // notifiy next alloc block, prev is free 
        // n_bp was already free, next_block prev bit already 0
        //SET_PRE_ALLOC(HDRP(NEXT_BLKP(bp)));
    
        // insert coalesce block into new size class
        list_insert(bp);
    }

    else if (!prev_alloc && next_alloc) {      // Case 3 // prev 
        // bp just freed, not in free list
        // remove prev contiguous block from its free list
        char *p_bp = (PREV_BLKP(bp));
        list_remove(p_bp);

        // coalsece block - Header, size allocate states
        size_t tsize = csize + GET_SIZE(HDRP(p_bp));
        PUT(HDRP(p_bp), PACK(tsize, GET_PRE_ALLOC(HDRP(p_bp)), 0));
        PUT(FTRP(p_bp), PACK(tsize, GET_PRE_ALLOC(HDRP(p_bp)), 0));

        // notifiy next alloc block, prev is free
        SET_PRE_ALLOC(HDRP(NEXT_BLKP(p_bp)));
        // insert coalesce block into new size class
        list_insert(p_bp);
        bp = p_bp;
    }

    else {                                     // Case 4 // Both 
        // bp just freed, not in free list
        // remove prev and next contiguous block from its free list
        char *p_bp = (PREV_BLKP(bp));
        list_remove(p_bp);
        char *n_bp = (NEXT_BLKP(bp));
        list_remove(n_bp);
        
        // coalsece block - Header, size allocate states
        size_t tsize = csize + GET_SIZE(HDRP(p_bp)) + GET_SIZE(HDRP(n_bp));
        PUT(HDRP(p_bp), PACK(tsize, GET_PRE_ALLOC(HDRP(p_bp)), 0));
        PUT(FTRP(p_bp), PACK(tsize, GET_PRE_ALLOC(HDRP(p_bp)), 0));

        // notifiy next alloc block, prev is free
        // n_bp was already free, next_block prev bit already 0
        //SET_PRE_ALLOC(HDRP(NEXT_BLKP(p_bp)));

        // insert coalesce block into new size class
        list_insert(p_bp);
        bp = p_bp;
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

static void mm_checkheap(int lineno) {
    // Free list level
    // For each free block make sure header == footer in size and allocation
    for(int class_index = 0; class_index < NCLASSES - 1; class_index++) {
        for (char *bp = HEADLIST(class_index); bp; bp = NEXT(bp)) { //iterate blocks
            if (GET_SIZE(HDRP(bp)) != GET_SIZE(FTRP(bp))) { // Size mismatch 
                printf("Size missmatch: Line %d", lineno);
            }
            if (GET_ALLOC(HDRP(bp)) != GET_ALLOC(FTRP(bp))) { // Alloc bit mismatch 
                printf("Allocation bit missmatch: Line %d\n", lineno);
            }
            if (GET_ALLOC(HDRP(bp)) == 1 || GET_ALLOC(FTRP(bp)) == 1) { // alloc block in freelist
                printf("Allocated block in freelist: Line %d\n", lineno);
            }
            if ((uintptr_t)bp % 8 != 0) { // Payload area is aligned 
                printf("Payload misaligned: Line %d\n", lineno);
            }
            if (find_sizeclass(GET_SIZE(HDRP(bp))) != class_index) { // Segregated list contain only blocks that beling to the size class
                printf("Free block in wrong size class: Line %d\n", lineno);
            }
            if (NEXT(bp) != NULL) { // Next/prev pointers in consecutive free blocks are consistent
                //check if bp->next->prev == bp 
                if (PREV(NEXT(bp)) != bp) {
                    printf("Next/Prev mismatch: Line %d\n", lineno);
                }
            }
        }    
    }


    // Heap level
    // Prologue/Epilogue blocks are at specific locations (e.g. heap boundaries and have special size/alloc fields)
    // All blocks stay in between the heap boundares

    char *epi_bp = (char *)mem_heap_hi() - 3;
    
    if (GET_SIZE((epi_bp)) != 0) { //Epilogue is at specific locations
        printf("Epilouge misplaced: Line %d\n", lineno);
    }

    if (GET_SIZE((epi_bp)) == 0 && !GET_ALLOC((epi_bp))) { //Epilogue never has free bit
        printf("Epilouge alloc bit = 0, should = 1: Line %d\n", lineno);
    }

    // hardcoded prolouge offsest 
    char *pro_bp = prologue_bp;
    if (GET_SIZE(HDRP(pro_bp)) != DSIZE) {
        printf("Prolouge header size error: Line %d\n", lineno);
    }
    if (GET_SIZE(((char *)pro_bp + WSIZE)) != DSIZE) {
        printf("Prolouge header size error: Line %d\n", lineno);
    }

    char *first_block = heap_listp + (21*WSIZE);
    for (char *bp = first_block; bp < epi_bp; bp = NEXT_BLKP(bp)) {
        if (!GET_ALLOC(HDRP(bp)) && !GET_ALLOC(HDRP(NEXT_BLKP(bp)))) { // No contigous free blocks in memory
            printf("Consecutive free/alloc blocks: Line %d\n", lineno);
        }
        if (GET_ALLOC(HDRP(bp)) != GET_PRE_ALLOC(HDRP(NEXT_BLKP(bp)))) {
            printf("Next block's Prev alloc bit is inncorrectly set: Line %d\n", lineno);
        }

        if (!GET_ALLOC(HDRP(bp))) { // All free blocks are in the free list
            // find in list 
            int is_include = 0;
            for (char *t_bp = HEADLIST(find_sizeclass(GET_SIZE(HDRP(bp)))); t_bp; t_bp = NEXT(t_bp)) { // Travse the size class
                if (bp == t_bp) {
                    is_include = 1;
                    break;
                }
            }
            if (!is_include) {
                printf("Free block not in free list: Line %d\n", lineno);
            }
        }
    }
    // No cycles in the list
    for (int i = 0; i < NCLASSES; i++) { //iterate all heads
        // Pass in head 
        // call cycle detection 
        if(loop_detection(HEADLIST(i))) {
            printf("Free list cycle detected: Line %d\n", lineno);
        }
    }

    // Other 
}

// returns if cycle detected 
static int loop_detection(char *bp) {
    char *slow = bp, *fast = bp;
    while (fast && NEXT(fast)) {
        slow = NEXT(slow);
        fast = NEXT(NEXT(fast));
        if (slow == fast) {
            return 1;
        }
    }
    return 0;
}












