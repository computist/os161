#include <synch_heap.h>
#include <synch.h>
#include <types.h>
#include <kern/errno.h>
#include <lib.h>

struct synch_heap {
    struct heap *h;
    struct lock *l;
    struct cv *c;
};

struct synch_heap* synch_heap_create(int(*comparator)(const void*, const void*)) {
    struct synch_heap * sh;

    sh = kmalloc(sizeof(struct synch_heap));
    if (sh == NULL) {
            return NULL;
    }

    sh->h = heap_create(comparator);
    if (sh->h==NULL) {
            kfree(sh);
            return NULL;
    }
    
    sh->l = lock_create("l");
    if (sh->l==NULL) {
            kfree(sh);
            kfree(sh->h);
            return NULL;
    }
    
    sh->c = cv_create("c");
    if (sh->c==NULL) {
            kfree(sh);
            kfree(sh->h);
            kfree(sh->l);
            return NULL;
    }
    
    return sh;
}

int synch_heap_push(struct synch_heap* sh, void* newval) {
    KASSERT(sh != NULL);
    
    lock_acquire(sh->l);
        int err = heap_push(sh->h, newval);
        if (err == ENOMEM) {
            lock_destroy(sh->l);
            cv_destroy(sh->c);
            kfree(sh);
            return ENOMEM;
        }
        cv_signal(sh->c, sh->l);
    lock_release(sh->l);
    return 0;
}

void* synch_heap_pop(struct synch_heap* sh) {
    KASSERT(sh != NULL);
    
    void* val = NULL;
    lock_acquire(sh->l);
        if(!heap_isempty(sh->h))
            val = heap_pop(sh->h);
    lock_release(sh->l);
    return val;
}

void* synch_heap_pop_wait(struct synch_heap* sh) {
    KASSERT(sh != NULL);
    
    void* val = NULL;
    lock_acquire(sh->l);
        while(heap_isempty(sh->h))
            cv_wait(sh->c, sh->l);
        val = heap_pop(sh->h);
    lock_release(sh->l);
    return val;
}

/*
void* synch_heap_front(struct synch_heap* sh) {

}

void* synch_heap_front_wait(struct synch_heap* sh) {

}*/

int synch_heap_isempty(struct synch_heap* sh) {
    KASSERT(sh != NULL);
    
    int val = 1;
    lock_acquire(sh->l);
        val = heap_isempty(sh->h);
    lock_release(sh->l);
    return val;
}

unsigned int synch_heap_getsize(struct synch_heap* sh) {
    KASSERT(sh != NULL);
    
    unsigned int val = 0;
    lock_acquire(sh->l);
        val = heap_getsize(sh->h);
    lock_release(sh->l);
    return val;
}

void synch_heap_destroy(struct synch_heap* sh) {
    KASSERT(sh != NULL);
    
    heap_destroy(sh->h);
    lock_destroy(sh->l);
    cv_destroy(sh->c);
    kfree(sh);
}

/*
void synch_heap_assertvalid(struct synch_heap* sh) {

}
*/
