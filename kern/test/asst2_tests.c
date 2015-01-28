#include <types.h>
#include <lib.h>
#include <clock.h>
#include <thread.h>
#include <synch.h>
#include <synch_queue.h>
#include <synch_heap.h>
#include <synch_hashtable.h>
#include <wchan.h>
#include <test.h>
#include <pid.h>
#include <asst2_tests.h>
#include <limits.h>
#include <fileops.h>
#include <proc.h>
#include <kern/fcntl.h>
#include <vnode.h>



/* PID tests */

// do an initial test: acquire - release - acquire
int test_minimal_acquire_release_acquire_counter(){

    kprintf("\n****** testing sequential counter acquire - release - acquire *******\n");

    int new_id = get_new_process_id();
    release_process_id(new_id);

    // the queue should have gotten this item, check if it is the same
    KASSERT(get_new_process_id() == new_id);

    // when acquiring a new item now, it should be larger
    KASSERT(get_new_process_id() > new_id);

    kprintf("\n******  done testing sequential counter acquire - release - acquire *******\n");

    return 0;

}


// test if PID counter reaches upper limit, get_new_process_id returns -1
int test_pid_upper_limit_counter(){
    kprintf("\n****** testing PID upper limit *******\n");

    // what is the current PID counter, probably 2
    kprintf("\nCurrent PID counter: %d \n", PID_counter);

    int last_pid = 0;

    bool pid_valid = true;

    // get new pid until we are full
    while(pid_valid){
        int c_pid = get_new_process_id();

        if(c_pid == -1){
            break;
        }
        last_pid = c_pid;           
    }

    // last pid should be the limit
    if(last_pid != __PID_MAX){
        kprintf("last PID was not PID_MAX, instead: %d\n", last_pid);
        return 1;
    }else{
        kprintf("last PID was %d\n",last_pid);
    }

    kprintf("\n****** done testing PID upper limit *******\n");

    return 0;

}

// test release of pids
int test_pid_release(){
    kprintf("\n****** testing PID release *******\n");

    // assert that PID should be full by now
    KASSERT(get_new_process_id() == -1);


    int pid_to_release = 10000; //10.000

    release_process_id(pid_to_release);


    // acquire new one, should now be 10.000 again
    KASSERT(get_new_process_id() == 10000);

    

    kprintf("\n****** done testing PID release *******\n");

    return 0;
}


// test upper limit of queue
int test_pid_upper_limit_queue(){
    kprintf("\n****** testing PID upper limit queueu *******\n");

    KASSERT(get_new_process_id() == -1);

    // release 1000 ids
    for(int i = 1000; i < 100 + 1000; i++){
        release_process_id(i);
    }

    // reaquire 1000 ids
    for(int i = 1000; i < 100 + 1000; i++){
        KASSERT(get_new_process_id() != -1);
    }


    kprintf("\n****** done testing PID upper limit queueu *******\n");

    return 0;
}


// do an final test: acquire - release - acquire
int test_minimal_acquire_release_acquire_queue(){

    kprintf("\n****** testing sequential queue acquire - release - acquire *******\n");

    // release one id
    release_process_id(100);

    int new_id = get_new_process_id();
    KASSERT(new_id != -1);
    release_process_id(new_id);

    // this is after the queue kicks in, so we should get the same PID
    KASSERT(get_new_process_id()  == new_id);

    kprintf("\n******  done testing sequential queue acquire - release - acquire *******\n");

    return 0;

}

// do an final test: acquire - release - acquire
int test_pid_in_use(){

	kprintf("\n****** testing PID in use *******\n");

	int pid = get_new_process_id();

	KASSERT(pid != -1);

	// pid should be available
	KASSERT(pidUsed(pid) == 1);

    	release_process_id(pid);

	// pid should not be available
	KASSERT(pidUsed(pid) == 0);

	kprintf("\n******  done testing PID in use *******\n");

	return 0;
}


// release ids in a range
void release_ids(int from, int to){
    for(int i = from; i < to; i++){
        release_process_id(i);
    }
}




// test file operations

// test file descriptor creation and destroy
int test_fd_create_destroy(){

    kprintf("\n****** testing create destroy of file descriptor table *******\n");

    struct file_descriptor *fd;
    fd = fd_create();
    KASSERT(fd!=NULL);
    fd_destroy(fd);

    kprintf("\n****** done create destroy of file descriptor table *******\n");

    return 0;

}

// test file descriptor table, rudimentary
int test_fd_table(){

    kprintf("\n****** testing create destroy of file table *******\n");

    // create a file descriptor table without a process
    struct fd_table* fdt;

    fdt = fd_table_create(NULL);
    KASSERT(fdt!=NULL);

    // add a file descriptor
    char filename[] = "con:";
    struct file_descriptor* fd = add_file_descriptor(fdt, filename, O_RDONLY);

    (void) fd;


    // destroy it again
    fd_table_destroy(fdt);
    

    kprintf("\n****** done testingt file table *******\n");
    return 0;
}


// rudimentary test synch hashtable
int test_synch_hashtable(){

    kprintf("\n****** testing synch hashmap *******\n");

    struct synch_hashtable *ht;

    ht = synch_hashtable_create();

    int val = 26;

    char key3[] = "felix";

    kprintf("test add \n");
    synch_hashtable_add(ht, key3, 5, &val);

    kprintf("test find \n");
    int res = *(int*)synch_hashtable_find(ht, key3, 5);
    kprintf("result %d \n", res);

    kprintf("test empty \n");
    KASSERT(synch_hashtable_isempty(ht) == 0);

    kprintf("test size \n");
    KASSERT(synch_hashtable_getsize(ht)==1);


    kprintf("test remove \n");
    synch_hashtable_remove(ht, key3, 5);

    KASSERT(synch_hashtable_isempty(ht) == 1);


    synch_hashtable_destroy(ht);

    kprintf("\n****** done testing synch hashmap *******\n");


    return 0;
}

/*

// tests fileoperations by creating a new process and doing some read write stuff
int test_file_ops(){


    kprintf("\n****** testing read  *******\n");

    // create a new process
    struct proc* p;
    char name[] =  "test";
    p = proc_create_runprogram(name);

    // the file table should have 3 entries now
    struct fd_table* fdt = p->p_fd_table;

    KASSERT(fdt->fds[0] != NULL);
    KASSERT(fdt->fds[0]->index == 0);
    KASSERT(fdt->fds[1] != NULL);
    KASSERT(fdt->fds[1]->index == 1);
    KASSERT(fdt->fds[2] != NULL);
    KASSERT(fdt->fds[2]->index == 2);    


    // let's write something to standard out
    char content[] = "hello\n";

    size_t written_bytes;

    fd_write(fdt->fds[1], content, 7, &written_bytes);

    KASSERT(written_bytes == 7);

    // test if we get the correct filedescriptor

    struct file_descriptor* stderr = get_fd(fdt, 2);

    fd_write(stderr, content, 7, &written_bytes);

    kprintf("read write test.txt\n");

    // create a new file descriptor for just a single text file
    char filename[] = "/test.txt";
    int fdID;
    int error = fd_open(fdt, filename, O_RDWR | O_CREAT, (int*) &fdID);

    (void) error;
    kprintf("file error: %d\n", error);

    struct file_descriptor* file = get_fd(fdt, fdID);

    fd_write(file, content, 7, &written_bytes);

    char read_buffer[50];
    bzero((void*) read_buffer, 50);


    fd_read(file, read_buffer, 7, &written_bytes);

    kprintf("\n ***  printing read stuff **** \n");

    kprintf(read_buffer);    


    fd_close(fdt, file);

    
    kprintf("\n ***  testing twiddle read **** \n");
    char filename_twiddle[] = "/a2a_filetest.txt";

    fd_open(fdt, filename_twiddle, O_RDONLY, (int*) &fdID);

    struct file_descriptor* file2 = get_fd(fdt, fdID);

    bzero((void*) read_buffer, 50);


    fd_read(file2, read_buffer, 40, &written_bytes);

    kprintf("\n ***  printing read stuff **** \n");

    kprintf(read_buffer); 

    (void) file;


//    kprintf("\n reading twiddle dee \n");
//    char filename[] = 

    
    kprintf("\n****** done testing write *******\n");




    return 0;

}






int test_file_table_copy(){


    kprintf("\n****** testing file table copy  *******\n");

    // create a new process
    struct proc* p;
    char name[] =  "test_fdt";
    p = proc_create_runprogram(name);

    // the file table should have 3 entries now
    struct fd_table* fdt = p->p_fd_table;

    char content[] = "hello\n";
    size_t written_bytes;

    // create a new file descriptor for just a single text file
    char filename[] = "/test.txt";
    int fdID;
    int error = fd_open(fdt, filename, O_RDWR | O_CREAT, (int*) &fdID);

    kprintf("file error: %d\n", error);

    struct file_descriptor* file = get_fd(fdt, fdID);

    fd_write(file, content, 7, &written_bytes);


    // copy the file descriptor
    struct file_descriptor* file_copy = fd_copy(file);

    // try to write
    fd_write(file_copy, content, 7, &written_bytes);

    // close the "parent" fdt
    fd_close(fdt, file);

    // try to write again
    fd_write(file_copy, content, 7, &written_bytes);

    KASSERT(file_copy->offset > 7); // since we already wrote a couple of times


    char console[] = "con:";
    int temp_id;
    // open and close 3 times
    fd_open(fdt, console, O_RDONLY, &temp_id);
    fd_close(fdt, fdt->fds[temp_id]);

    fd_open(fdt, console, O_RDONLY, &temp_id);
    fd_close(fdt, fdt->fds[temp_id]);
    
    fd_open(fdt, console, O_RDONLY, &temp_id);
    fd_close(fdt, fdt->fds[temp_id]);


    // copy the current file table
    struct fd_table* fdt_copy = fd_table_copy(fdt, p);

    // open two new files, they should have the same id

    int old_fd_id, new_fd_id;

    fd_open(fdt, console, O_RDONLY, &old_fd_id);
    fd_open(fdt_copy, console, O_RDONLY, &new_fd_id);

    kprintf("old id: %d, new id: %d\n", old_fd_id, new_fd_id);

    KASSERT(old_fd_id == new_fd_id);


    (void) file;
    (void) file_copy;
    
    kprintf("\n****** testing file table copy *******\n");

    return 0;

}


*/



int asst2_tests(int nargs, char **args){

	(void) nargs;
	(void) args;

	kprintf("starting tests for PID");
	//KASSERT(test_pid_in_use() == 0);
	// DO NOT CHANGE THE ORDER HERE!
	
	//KASSERT(test_minimal_acquire_release_acquire_counter() == 0);
	//KASSERT(test_pid_upper_limit_counter() == 0);
	//KASSERT(test_pid_release() == 0);    
	//KASSERT(test_minimal_acquire_release_acquire_queue() == 0);

    
	//release_ids(10000,30000);

    kprintf("starting tests for hash table \n");
	//test_synch_hashtable();


	kprintf("starting tests for files ops \n");
	//test_fd_create_destroy();    
	//test_fd_table();
    //test_file_ops();
    //test_file_table_copy();


    kprintf("\n\n");
    kprintf("**************************** \n");
    kprintf("**************************** \n");
    kprintf("**************************** \n");
    kprintf("*********         ********** \n");
    kprintf("*********   OK    ********** \n");
    kprintf("*********         ********** \n");
    kprintf("**************************** \n");
    kprintf("**************************** \n");
    kprintf("**************************** \n");
    kprintf("\n\n");
	return 0;
}
