/**
The Process Identification Stuff


**/

#include <pid.h>
#include <list.h>
#include <limits.h>
#include <queue.h>
#include <synch.h>
#include <lib.h>


// this will be incremented until it reaches the limit defined in os161/kern/include/kern/limits.h
// 0,1,2 should be reserved for stdin, stdout and stderr. __PID_MIN is 2 and we will increase PID_COUNTER before first return
int PID_counter = __PID_MIN;

struct queue *PID_queue;
struct lock *l;
struct list *lst_usedPIDs;

// function to compare pids
static
int
int_comparator(void* left, void* right)
{
    int l = *(int*)left;
    int r = *(int*)right;
    return l-r;
}

// returns a new process id or -1 if no more process ids
int get_new_process_id(){
	int new_id = -1;

	lock_acquire(l);

	// see if we can recycle an ID (we do this first because otherwise we will get a huge queue of old IDs)
	if (!queue_isempty(PID_queue)){
		new_id = (int) queue_front(PID_queue);
		// remove that item from the queue
		queue_pop(PID_queue);
	}else{
		// otherwise, see if we can increment the PID counter
		if(PID_counter < __PID_MAX){
			// we can increment
			PID_counter++;

			// return that new value
			new_id = PID_counter;
		} // else, we will just return new_id which should be -1
	}


	if(new_id >= 0){
		int ret = list_push_back(lst_usedPIDs, (void*) &new_id);
		if(ret){
			lock_release(l);
			return ret;
		}
	}


	lock_release(l);

	return new_id;
};


// releases a used process id to be reused
void release_process_id(int i){

	lock_acquire(l);

	queue_push(PID_queue, (void*) i);

	// remove pid from used list
	list_remove(lst_usedPIDs, (void*) &i, &int_comparator);
	//int *ret = *(int*)list_remove(lst_usedPIDs, (void*) i, &int_comparator);		//TODO change return to check pass fail
	//if(res == NULL || *ret != i)		
	//	return ret;

	lock_release(l);
};


// initializes the PID system 
void pid_bootstrap(){

	// intialize the queue
	PID_queue = queue_create();

	// intialize the lock
	l = lock_create("pid_lock");

	// initialize list for used pids
	lst_usedPIDs = list_create();
};


// cleanup the PID system 
void pid_cleanup(){

	//list_destroy(lst_usedPIDs);
	queue_destroy(PID_queue);
	lock_destroy(l);
};


// check if pid is in use
int pidUsed(int pid){
	lock_acquire(l);
	int *res;
	res = (int*)list_find(lst_usedPIDs, (void*) &pid, &int_comparator);
    	lock_release(l);
	if (res != NULL && *res == pid)
		return 1;
	else
		return 0;	
};





