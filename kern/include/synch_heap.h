#ifndef _SYNCH_HEAP_H_
#define _SYNCH_HEAP_H_

#include <heap.h>

struct synch_heap;

struct synch_heap* synch_heap_create(int(*comparator)(const void*, const void*));
int synch_heap_push(struct synch_heap* sh, void* newval);
void* synch_heap_pop(struct synch_heap* sh);
void* synch_heap_pop_wait(struct synch_heap* sh);
/*void* synch_heap_front(struct synch_heap* sh);
void* synch_heap_front_wait(struct synch_heap* sh);*/
int synch_heap_isempty(struct synch_heap* sh);
unsigned int synch_heap_getsize(struct synch_heap* sh);
void synch_heap_destroy(struct synch_heap* sh);
//void synch_heap_assertvalid(struct synch_heap* sh);

#endif /* _SYNCH_HEAP_H_ */
