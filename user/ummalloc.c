#include "kernel/types.h"

//
#include "user/user.h"

//
#include "ummalloc.h"

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT - 1)) & ~0x7)

#define SIZE_T_SIZE (ALIGN(sizeof(uint)))

/* 
 * refer to CSAPP Malloc Lab
 * refer to https://blog.csdn.net/weixin_47089544/article/details/124184251
 */

#define WSIZE 4
#define DSIZE 8
#define MINSIZE 8
#define CHUNKSIZE (1 << 12)

#define MAX(x, y) ((x) > (y)? (x): (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(uint *)(p))
#define SET(p, val) ((*(uint *)(p)) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(p) ((void*)(p) - WSIZE)
#define FTRP(p) ((void*)(p) + GET_SIZE(HDRP(p)) - DSIZE)

#define PREV_BLKP(p) ((void*)(p) - GET_SIZE((void*)(p) - DSIZE))
#define NEXT_BLKP(p) ((void*)(p) + GET_SIZE((void*)(p) - WSIZE))

void* head;

/*
 * mm_init - initialize the malloc package.
 */
int mm_init(void) { 
    head = sbrk(4 * WSIZE);
    if (head == (void*)-1)
        return 0;

    SET(head, PACK(0, 0));
    SET(head + WSIZE, PACK(8, 1));
    SET(head + 2 * WSIZE, PACK(8, 1));
    SET(head + 3 * WSIZE, PACK(0, 1));

    head += 2 * WSIZE;
    
    return 0;
}

void* extend_heap(uint size) {
    // printf("extend heap\n");
    size = ALIGN(size);
    void* ptr = sbrk(size);
    if (ptr == (void*)-1) return 0;

    SET(HDRP(ptr), PACK(size, 0));
    SET(FTRP(ptr), PACK(size, 0));
    SET(HDRP(NEXT_BLKP(ptr)), PACK(0, 1));
    
    return merge(ptr);
}

void* find_fit(uint size) {
    for (void* p = head; GET_SIZE(HDRP(p)) != 0; p = NEXT_BLKP(p)) 
        if (!GET_ALLOC(HDRP(p)) && GET_SIZE(HDRP(p)) >= size)
            return p;
    return 0;
}

void place(void* ptr, uint new_size) {
    // printf("place\n");
    uint size = GET_SIZE(HDRP(ptr));
    if (size - new_size >= MINSIZE) {
        SET(HDRP(ptr), PACK(new_size, 1));
        SET(FTRP(ptr), PACK(new_size, 1));
        void* next_ptr = NEXT_BLKP(ptr);
        SET(HDRP(next_ptr), PACK(size - new_size, 0));
        SET(FTRP(next_ptr), PACK(size - new_size, 0));
    } else {
        SET(HDRP(ptr), PACK(size, 1));
        SET(FTRP(ptr), PACK(size, 1));
    }
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(uint size) {
    // printf("malloc\n");
    if (size == 0) return 0;
    size = ALIGN(size + 2 * WSIZE);

    void* ptr = find_fit(size);
    if (ptr != 0) {
        place(ptr, size);
        return ptr;
    }

    ptr = extend_heap(MAX(size, CHUNKSIZE));
    if (ptr == 0) return 0;
    place(ptr, size);
    return ptr;
}

void* merge(void* ptr) {
    // printf("merge\n");
    void* prev_ptr = PREV_BLKP(ptr);
    void* next_ptr = NEXT_BLKP(ptr);
    uint size = GET_SIZE(HDRP(ptr)), 
         alloc_prev = GET_ALLOC(HDRP(prev_ptr)),
         alloc_next = GET_ALLOC(HDRP(next_ptr));
    if (alloc_prev && alloc_next) {
        return ptr;
    } else if (!alloc_prev && alloc_next) {
        size += GET_SIZE(HDRP(prev_ptr));
        SET(HDRP(prev_ptr), PACK(size, 0));
        SET(FTRP(prev_ptr), PACK(size, 0));
        return prev_ptr;
    } else if (alloc_prev && !alloc_next) {
        size += GET_SIZE(HDRP(next_ptr));
        SET(HDRP(ptr), PACK(size, 0));
        SET(FTRP(ptr), PACK(size, 0));
        return ptr;
    } else {
        size += GET_SIZE(HDRP(prev_ptr)) + GET_SIZE(HDRP(next_ptr));
        SET(HDRP(prev_ptr), PACK(size, 0));
        SET(FTRP(prev_ptr), PACK(size, 0));
        return prev_ptr;
    }
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr) {
    uint size = GET_SIZE(HDRP(ptr));
    SET(HDRP(ptr), PACK(size, 0));
    SET(FTRP(ptr), PACK(size, 0));
    merge(ptr);
    return ;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, uint size) {
    void *old_ptr = ptr;

    if (ptr == 0) return mm_malloc(size);
    if (size == 0) {
        mm_free(ptr);
        return 0;
    }

    uint new_size = ALIGN(size + 2 * WSIZE);
    uint old_size = GET_SIZE(HDRP(old_ptr));

    if (new_size == old_size) return ptr;
    else if (new_size < old_size) {
        place(ptr, new_size);
        return ptr;
    } else {
        void* next_ptr = NEXT_BLKP(ptr);
        if (!GET_ALLOC(HDRP(next_ptr)) && GET_SIZE(HDRP(next_ptr)) + old_size >= new_size) {
            old_size += GET_SIZE(HDRP(next_ptr));
            SET(HDRP(ptr), PACK(old_size, 1));
            SET(FTRP(ptr), PACK(old_size, 1));
            return ptr;
        } else {
            void* new_ptr = find_fit(new_size);
            if (new_ptr == 0) {
                new_ptr = extend_heap(MAX(new_size, CHUNKSIZE));
                if (new_ptr == 0) return 0;
            }
            place(new_ptr, new_size);
            memcpy(new_ptr, old_ptr, old_size - 2 * WSIZE);
            mm_free(old_ptr);
            return new_ptr;
        }
    }
    return 0;
}
