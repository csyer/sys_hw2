extern int mm_init(void);
extern void *mm_malloc(uint size);
extern void mm_free(void *ptr);
extern void *mm_realloc(void *ptr, uint size);

extern void* extend_heap(uint size);
extern void* find_fit(uint size);
extern void place(void* ptr, uint newsize);
extern void* merge(void* ptr);