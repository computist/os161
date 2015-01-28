#include <synch_queue.h>
#include <synch.h>
#include <types.h>
#include <kern/errno.h>
#include <lib.h>

struct synch_queue {
    struct queue *q;
    struct lock *l;
    struct cv *c;
};

struct synch_queue* synch_queue_create(void) {
    struct synch_queue * sq;

    sq = kmalloc(sizeof(struct synch_queue));
    if (sq == NULL) {
            return NULL;
    }

    sq->q = queue_create();
    if (sq->q==NULL) {
            kfree(sq);
            return NULL;
    }
    
    sq->l = lock_create("l");
    if (sq->l==NULL) {
            kfree(sq);
            kfree(sq->q);
            return NULL;
    }
    
    sq->c = cv_create("c");
    if (sq->c==NULL) {
            kfree(sq);
            kfree(sq->q);
            kfree(sq->l);
            return NULL;
    }
    
    return sq;
}

int synch_queue_push(struct synch_queue* sq, void* newval) {
    KASSERT(sq != NULL);
    
    lock_acquire(sq->l);
        int err = queue_push(sq->q, newval);
        if (err == ENOMEM) {
            //queue doesn't destroy itself on ENOMEM
            //synch_queue_destroy(sh);
            //but that might change, better to leak a bit
            lock_destroy(sq->l);
            cv_destroy(sq->c);
            kfree(sq);
            return ENOMEM;
        }
        cv_signal(sq->c, sq->l);
    lock_release(sq->l);
    return 0;
}

void* synch_queue_pop(struct synch_queue* sq) {
    KASSERT(sq != NULL);

    void* val = NULL;
    lock_acquire(sq->l);
        if(!queue_isempty(sq->q)) {
            val = queue_front(sq->q);
            queue_pop(sq->q);
        }
    lock_release(sq->l);
    return val;
}

void* synch_queue_pop_wait(struct synch_queue* sq) {
    KASSERT(sq != NULL);
    
    void* val = NULL;
    lock_acquire(sq->l);
        while(queue_isempty(sq->q))
            cv_wait(sq->c, sq->l);
        val = queue_front(sq->q);
        if(val != NULL)
            queue_pop(sq->q);
    lock_release(sq->l);
    return val;
}

/*
void* synch_queue_front(struct synch_queue* sq) {

}

void* synch_queue_front_wait(struct synch_queue* sq) {

}*/

int synch_queue_isempty(struct synch_queue* sq) {
    KASSERT(sq != NULL);
    
    int val = 1;
    lock_acquire(sq->l);
        val = queue_isempty(sq->q);
    lock_release(sq->l);
    return val;
}

unsigned int synch_queue_getsize(struct synch_queue* sq) {
    KASSERT(sq != NULL);
    
    unsigned int val = 0;
    lock_acquire(sq->l);
        val = queue_getsize(sq->q);
    lock_release(sq->l);
    return val;
}

void synch_queue_destroy(struct synch_queue* sq) {
    KASSERT(sq != NULL);
    
    queue_destroy(sq->q);
    lock_destroy(sq->l);
    cv_destroy(sq->c);
    kfree(sq);
}

/*
void synch_queue_assertvalid(struct synch_queue* sq) {

}
*/
