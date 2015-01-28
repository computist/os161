#include <hashtable.h>
#include <synch_hashtable.h>
#include <list.h>
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <synch.h>






struct synch_hashtable* synch_hashtable_create(){

	struct synch_hashtable *ht;
	ht = kmalloc(sizeof(struct synch_hashtable));
	
	if(ht==NULL){
		return NULL;
	}

	// create the internal hashtable
	ht->ht = hashtable_create();
	if(ht->ht == NULL){
		kfree(ht);
		return NULL;
	}

	//create the lock
	ht->lock = lock_create("hashtable lock");
	if(ht->lock== NULL){
		hashtable_destroy(ht->ht);
		kfree(ht);
		return NULL;
	}

	return ht;
};

void synch_hashtable_destroy(struct synch_hashtable* ht){

	hashtable_destroy(ht->ht);
	lock_destroy(ht->lock);

	kfree(ht);
}



int synch_hashtable_add(struct synch_hashtable* h, char* key, unsigned int keylen, void* val){

	int ret;

	lock_acquire(h->lock);

	ret = hashtable_add(h->ht, key, keylen, val);

	lock_release(h->lock);

	return ret;
}



void* synch_hashtable_find(struct synch_hashtable* h, char* key, unsigned int keylen){

	void *ret;

	lock_acquire(h->lock);
	ret = hashtable_find(h->ht, key, keylen);
	lock_release(h->lock);

	return ret;
}


void* synch_hashtable_remove(struct synch_hashtable* h, char* key, unsigned int keylen){
	void* ret;	

	lock_acquire(h->lock);
	ret = hashtable_remove(h->ht, key, keylen);
	lock_release(h->lock);

	return ret;
}


int synch_hashtable_isempty(struct synch_hashtable* h){

	int ret;

	lock_acquire(h->lock);

	ret = hashtable_isempty(h->ht);

	lock_release(h->lock);

	return ret;
}


unsigned int synch_hashtable_getsize(struct synch_hashtable* h){

	unsigned int ret;

	lock_acquire(h->lock);
	ret = hashtable_getsize(h->ht);
	lock_release(h->lock);

	return ret;
}


void synch_hashtable_assertvalid(struct synch_hashtable* h){
	lock_acquire(h->lock);
	hashtable_assertvalid(h->ht);
	lock_release(h->lock);
}




