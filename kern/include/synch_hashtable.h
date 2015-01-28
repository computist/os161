#ifndef _S_HASHTABLE_H_
#define _S_HASHTABLE_H_





struct synch_hashtable{

	struct hashtable *ht;
	struct lock *lock;


};

struct synch_hashtable* synch_hashtable_create(void);


int synch_hashtable_add(struct synch_hashtable* h, char* key, unsigned int keylen, void* val);
void* synch_hashtable_find(struct synch_hashtable* h, char* key, unsigned int keylen);
void* synch_hashtable_remove(struct synch_hashtable* h, char* key, unsigned int keylen);
int synch_hashtable_isempty(struct synch_hashtable* h);
unsigned int synch_hashtable_getsize(struct synch_hashtable* h);
void synch_hashtable_destroy(struct synch_hashtable* h);
void synch_hashtable_assertvalid(struct synch_hashtable* h);



#endif
