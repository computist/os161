#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <test.h>
#include <cust_locktest.h>



#define NTHREADS      32


//static struct semaphore *donesem; // is used to synchronize threads since we can not join jet



////////////// START CUSTOM LOCK TESTS












/*
// test if I can release a lock that I don't hold
static int locktest_holder_single_helper(void *junk, unsigned long num){

    (void) junk;

    lock_release(testlock);
    fail("Kernel should have paniced", num);

    return 0;

}


static void locktest_holder_single(){

    // acquire lock on this thread 
    lock_acquire(testlock);

    // create new thread
    int result = thread_fork("holder_test_single", NULL, NULL, locktest_holder_single_helper, NULL, 0);
        if (result) {
            panic("locktest: thread_fork failed: %s\n",
                  strerror(result));
        }




}

*/


/////////////////////////////////
//////////////// THREAD TESTS
/////////////////////////////////



static int thread_counter = 0;



// helper function for thread_test_basic_join
static int thread_test_basic_join_helper(void *junk, unsigned long num){

    (void) junk;
    (void) num;


    kprintf("I live and run!");

   // thread_yield();
   
    for(int i = 0; i< 100; i++){
     kprintf("test");


    } 


    thread_counter++;
    
    thread_exit(0);

}


// simply tests if a thread really joins its parent
static int thread_test_basic_join(){

    int result;
    int return_value;

    
    struct thread *temp_thread;
	
    kprintf("starting thread");

    result = thread_fork("simple_join", &temp_thread, NULL, &thread_test_basic_join_helper, NULL, 1);
    if (result) {
        panic("thread_test_basic_join: thread_fork failed: %s\n",  strerror(result));
    }

    kprintf("thread forked");
	kprintf(temp_thread->t_name);
    thread_join(temp_thread, &return_value);
    kprintf("thread joined");


    // our child thread should have counted to 1 at this point
    KASSERT(thread_counter == 1);
    KASSERT(return_value == 0);

    

    return 0;
}






// execute all tests in this testsuite
int
locktest_extended(int nargs, char **args)
{

    (void) nargs;
    (void) args;

    KASSERT(thread_test_basic_join() == 0);

    return 0;



}
