#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <synch_queue.h>
#include <synch_heap.h>
#include <wchan.h>
#include <test.h>

static struct lock *testlock;
static struct cv *testcv;
static struct semaphore *donesem;
static volatile unsigned long testval;


static struct synch_queue *testsq;
static struct synch_heap *testsh;

// variables for lock testings
#define NTHREADS      32
static struct semaphore *donesem_locks; // is used to synchronize threads since we can not join jet
static struct lock *testlock;



////////////////////////////////
////////////// SYNCH QUEUE TESTS
////////////////////////////////

static void initsq(void)
{
    if(testsq == NULL) {
        testsq = synch_queue_create();
        if(testsq == NULL) {
            panic("queuetest: synch_queue_create failed\n");
        }
    }
    if (donesem==NULL) {
        donesem = sem_create("donesem", 0);
        if (donesem == NULL) {
            panic("synchtest: sem_create failed\n");
        }
    }
}

//make sure it's FIFO and pushing/popping doesn't error out
int sqtest_fifo(int nargs, char **args)
{
    (void)nargs;
    (void)args;
    int i;
    initsq();
    kprintf("Starting sq fifo test...\n");
    
    for(i = 0; i < 5; i++) {
        int err = synch_queue_push(testsq, (void*)i);
        if (err != 0) {
            kprintf("err: %d", err);
            return err;
        }
    }
    for(i = 0; i < 5; i++) {
        KASSERT((int)synch_queue_pop(testsq) == i);
    }
    
    kprintf("sq fifo test done\n");
    return 0;
}

//thread calls synch_queue_pop_wait and semaphores
static int sq_wait_thread(void *junk, unsigned long num)
{
    (void)junk;
    (void)num;
    V(donesem);
    synch_queue_pop_wait(testsq);
    V(donesem);
    return 0;
}

//pop_wait stops until something is pushed
int sqtest_wait(int nargs, char **args)
{
    (void)nargs;
    (void)args;
    int result;
    initsq();
    kprintf("Starting sq wait test...\n");
    
    //create pop_wait thread
    result = thread_fork("sqtest", NULL, NULL, sq_wait_thread, NULL, 0);
    if (result) {
        panic("sqtest: thread_fork failed: %s\n",
              strerror(result));
    }
    
    //ensure other thread is waiting
    P(donesem);
    KASSERT(donesem->sem_count == 0);
    
    //push and wake other thread
    synch_queue_push(testsq, (void*)2);
    
    //other thread is done
    P(donesem);
    
    kprintf("sq wait test done\n");
    return 0;
}

/* less comparator for int */
static int int_lessthan(const void* left, const void* right) {
    int l = *(int*)left;
    int r = *(int*)right;
    return (l < r);
}


////////////////////////////////
////////////// SYNCH HEAP TESTS
////////////////////////////////

static void initsh(void)
{
    if(testsh == NULL) {
        testsh = synch_heap_create(&int_lessthan);
        if(testsh == NULL) {
            panic("heaptest: synch_heap_create failed\n");
        }
    }
    if (donesem==NULL) {
        donesem = sem_create("donesem", 0);
        if (donesem == NULL) {
            panic("heaptest: sem_create failed\n");
        }
    }
}

//thread calls synch_heap push and semaphores
static int sh_order_thread(void *junk, unsigned long num)
{
    (void)junk;
    int * elem = (int*)kmalloc(sizeof(int));
    *elem = num;
    KASSERT(synch_heap_push(testsh, (void*)elem) == 0);
    V(donesem);
    return 0;
}

//make sure it orders correctly and pushing/popping doesn't error out
int shtest_order(int nargs, char **args)
{
    (void)nargs;
    (void)args;
    int i, result, *elem;
    initsh();
    kprintf("Starting sh order test...\n");
    
    //create push threads
    result = thread_fork("shtest", NULL, NULL, sh_order_thread, NULL, 1);
    if (result) {
        panic("shtest: thread_fork failed: %s\n",
              strerror(result));
    }
    //create push threads
    result = thread_fork("shtest", NULL, NULL, sh_order_thread, NULL, 0);
    if (result) {
        panic("shtest: thread_fork failed: %s\n",
              strerror(result));
    }
    //create push threads
    result = thread_fork("shtest", NULL, NULL, sh_order_thread, NULL, 3);
    if (result) {
        panic("shtest: thread_fork failed: %s\n",
              strerror(result));
    }
    //create push threads
    result = thread_fork("shtest", NULL, NULL, sh_order_thread, NULL, 2);
    if (result) {
        panic("shtest: thread_fork failed: %s\n",
              strerror(result));
    }
    
    for(i = 0; i < 4; i++)
        P(donesem);
    for(i = 0; i < 4; i++) {
        elem = (int*)synch_heap_pop(testsh);
        KASSERT(*elem == i);
        kfree(elem);
    }
    
    kprintf("sh order test done\n");
    return 0;
}

//thread calls synch_heap_pop_wait and semaphores
static int sh_wait_thread(void *junk, unsigned long num)
{
    (void)junk;
    (void)num;
    V(donesem);
    int * elem = (int*)synch_heap_pop_wait(testsh);
    kfree(elem);
    V(donesem);
    return 0;
}

//pop_wait stops until something is pushed
int shtest_wait(int nargs, char **args)
{
    (void)nargs;
    (void)args;
    int result;
    initsh();
    kprintf("Starting sh wait test...\n");
    
    //create pop_wait thread
    result = thread_fork("shtest", NULL, NULL, sh_wait_thread, NULL, 0);
    if (result) {
        panic("shtest: thread_fork failed: %s\n",
              strerror(result));
    }
    
    //ensure other thread is waiting
    P(donesem);
    KASSERT(donesem->sem_count == 0);
    
    //push and wake other thread
    int * elem = (int*)kmalloc(sizeof(int));
    *elem = 2;
    KASSERT(synch_heap_push(testsh, (void*)elem) == 0);
    
    //other thread is done
    P(donesem);
    
    kprintf("sh wait test done\n");
    return 0;
}



///////////////////////////////////////
////////////// CONDITION VARIABLE TESTS
///////////////////////////////////////


static void initcv(void)
{
    testval = 0;

    if (testlock==NULL) {
        testlock = lock_create("testlock");
        if (testlock == NULL) {
            panic("synchtest: lock_create failed\n");
        }
    }
    if (testcv==NULL) {
        testcv = cv_create("testlock");
        if (testcv == NULL) {
            panic("synchtest: cv_create failed\n");
        }
    }
    if (donesem==NULL) {
        donesem = sem_create("donesem", 0);
        if (donesem == NULL) {
            panic("synchtest: sem_create failed\n");
        }
    }
}

static int cvtest_thread(void *junk, unsigned long num)
{
    (void)junk;
    lock_acquire(testlock);
    V(donesem);
    while(testval != num)
        cv_wait(testcv, testlock);
    V(donesem);
    lock_release(testlock);
    return 0;
}

//single thread wait/signal
int cvtest_wait(int nargs, char **args)
{
    (void)nargs;
    (void)args;
    int result;
    initcv();
    kprintf("Starting CV wait test...\n");
    
    //create a thread that'll just wait until testval == 1
    result = thread_fork("cvtest", NULL, NULL, cvtest_thread, NULL, 1);
    if (result) {
        panic("cvtest: thread_fork failed: %s\n",
              strerror(result));
    }
    P(donesem);
    //other thread is waiting
    
    lock_acquire(testlock);
        //other thread should be in the wchan's waitlist
        //KASSERT(!wchan_isempty(testcv->cv_wchan, &testcv->cv_lock));
        
        //increment testval to wake the thread
        testval++;
        cv_signal(testcv, testlock);
    lock_release(testlock);
    P(donesem);
    
    //thread should've woken up
    //KASSERT(wchan_isempty(testcv->cv_wchan, &testcv->cv_lock));
    
    kprintf("CV wait test done\n");
    return 0;
}

//cv_signal only wakes 1 thread
int cvtest_signal(int nargs, char **args)
{
    (void)nargs;
    (void)args;
    int result;
    initcv();
    kprintf("Starting CV signal test...\n");
    
    //create threads waiting for testval == 1
    result = thread_fork("cvtest", NULL, NULL, cvtest_thread, NULL, 1);
    if (result) {
        panic("cvtest: thread_fork failed: %s\n",
              strerror(result));
    }
    result = thread_fork("cvtest", NULL, NULL, cvtest_thread, NULL, 1);
    if (result) {
        panic("cvtest: thread_fork failed: %s\n",
              strerror(result));
    }
    P(donesem);
    P(donesem);
    //threads are now waiting
    
    //increment testval so either could wake, but only signal one
    lock_acquire(testlock);
        testval++;
        cv_signal(testcv, testlock);
    lock_release(testlock);
    P(donesem);
    
    lock_acquire(testlock);
        //should still be one thread waiting
        //KASSERT(!wchan_isempty(testcv->cv_wchan, &testcv->cv_lock));
        
        //signal the other thread
        cv_signal(testcv, testlock);
    lock_release(testlock);
    P(donesem);
    
    //all threads should've woken up
    //KASSERT(wchan_isempty(testcv->cv_wchan, &testcv->cv_lock));
    
    kprintf("CV signal test done\n");
    return 0;
}

//cv_broadcast wakes all threads
int cvtest_broadcast(int nargs, char **args)
{
    (void)nargs;
    (void)args;
    int result;
    initcv();
    kprintf("Starting CV broadcast test...\n");
    
    //create two threads waiting on testval == 1
    result = thread_fork("cvtest", NULL, NULL, cvtest_thread, NULL, 1);
    if (result) {
        panic("cvtest: thread_fork failed: %s\n",
              strerror(result));
    }
    result = thread_fork("cvtest", NULL, NULL, cvtest_thread, NULL, 1);
    if (result) {
        panic("cvtest: thread_fork failed: %s\n",
              strerror(result));
    }
    
    //create one thread waiting on testval == 2
    result = thread_fork("cvtest", NULL, NULL, cvtest_thread, NULL, 2);
    if (result) {
        panic("cvtest: thread_fork failed: %s\n",
              strerror(result));
    }
    
    P(donesem);
    P(donesem);
    P(donesem);
    //threads are now waiting
    
    //increment testval and then broadcast the change so two threads should wake up and call V()
    lock_acquire(testlock);
        testval++;
        cv_broadcast(testcv, testlock);
    lock_release(testlock);
    P(donesem);
    P(donesem);
    
    lock_acquire(testlock);
        //should still be one thread waiting
        //KASSERT(!wchan_isempty(testcv->cv_wchan, &testcv->cv_lock));
        
        //increment again, waking the last thread
        testval++;
        cv_broadcast(testcv, testlock);
    lock_release(testlock);
    P(donesem);
    
    //the cv shouldn't have any waiting threads
    //KASSERT(wchan_isempty(testcv->cv_wchan, &testcv->cv_lock));
    
    kprintf("CV broadcast test done\n");
    return 0;
}



////////////////////////////////
//////////////////// LOCK TESTS
////////////////////////////////



// used to fail during lock test
static int fail(const char *msg, unsigned long num)
{
    kprintf("thread %lu: %s\n ",num, msg);
    kprintf("Test failed\n");

    lock_release(testlock);

    V(donesem_locks);
    thread_exit(0);
}

// this is run by the locktest holder threads
static int locktest_holder_helper_function(void *junk, unsigned long num){

    (void)junk;


    // be sure that we are not the lock holder
    if(lock_do_i_hold(testlock)){
        fail("I should not hold the lock", num);
    }

    // try to acquire the lock
    lock_acquire(testlock);

        // i should have the lock now
        if(!lock_do_i_hold(testlock)){
            fail("I should hold the lock", num);
        }

        // yield for other pro cesses
        thread_yield();

        // check again for the holder
        if(!lock_do_i_hold(testlock)){
            fail("I should hold the lock (after yield)", num);
        }

    // release the lock
    lock_release(testlock);

    // be sure that we are not the lock holder
    if(lock_do_i_hold(testlock)){
        fail("I should not hold the lock (after release)", num);
    }



    V(donesem_locks);

    return 0;
}


// tests if the holder is always correct
static int locktest_holder_multiple()
{
    
    int result;   

    donesem_locks = sem_create("lock_sem",0);
    
    kprintf("Starting lock holder test  (multiple)...\n");

    for (int i=0; i<NTHREADS; i++) {
        result = thread_fork("holder_test", NULL, NULL, locktest_holder_helper_function, NULL, i);
        if (result) {
            panic("locktest: thread_fork failed: %s\n",
                  strerror(result));
        }
    }
    for (int i=0; i<NTHREADS; i++) {
        P(donesem_locks);
    }

    kprintf("Lock holder test done. (multiple)\n");

    return 0;
}


/* for thread_join tests */
static
int 
go(void* p, unsigned long n)
{
	(void)p; //unused

	kprintf("Hello from thread %ld\n", n);
	return 100 + n;
}

static
void
runthreadjointest(void)
{
	struct thread *children[NTHREADS];
	int err;
	int ret;
	
	for(int i=0; i < NTHREADS; i++)
		thread_fork("child", &(children[i]), NULL, &go, NULL, i);



	for(int i=0; i < NTHREADS; i++)
	{
		err = thread_join(children[i], &ret);
		kprintf("Thread %d returned with %d\n", i, ret);
	}

	kprintf("Main thread done.\n");
}



// thest for thread_join
int
threadtest4(int nargs, char **args)
{
    (void) nargs;
    (void) args;

    kprintf("Starting thread test 4 (join)...\n");
    runthreadjointest();
    kprintf("\nThread test 4 (join) done.\n");

    return 0;
}




int asst1_tests(int nargs, char **args)
{
    KASSERT(cvtest_wait(nargs, args) == 0);
    KASSERT(cvtest_signal(nargs, args) == 0);
    KASSERT(cvtest_broadcast(nargs, args) == 0);
    KASSERT(sqtest_fifo(nargs, args) == 0);
    KASSERT(sqtest_wait(nargs, args) == 0);
    KASSERT(shtest_order(nargs, args) == 0);
    KASSERT(shtest_wait(nargs, args) == 0);

    KASSERT(locktest_holder_multiple(nargs, args) == 0);
 
    KASSERT(threadtest4(nargs, args)==0);

    return 0;
}
