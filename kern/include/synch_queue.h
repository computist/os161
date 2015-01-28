#ifndef _SYNCH_QUEUE_H_
#define _SYNCH_QUEUE_H_

#include <queue.h>

struct synch_queue;

struct synch_queue* synch_queue_create(void);
int synch_queue_push(struct synch_queue* sq, void* newval);
void* synch_queue_pop(struct synch_queue* sq);
void* synch_queue_pop_wait(struct synch_queue* sq);
/*void* synch_queue_front(struct synch_queue* sq);
void* synch_queue_front_wait(struct synch_queue* sq);*/
int synch_queue_isempty(struct synch_queue* sq);
unsigned int synch_queue_getsize(struct synch_queue* sq);
void synch_queue_destroy(struct synch_queue* sq);
//void synch_queue_assertvalid(struct synch_queue* sq);

#endif /* _SYNCH_QUEUE_H_ */
