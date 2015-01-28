#ifndef _PID_H_
#define _PID_H_


int PID_counter;
//struct queue *PID_queue;
//struct lock *l;


// returns a new process id or -1 if no more process ids
int get_new_process_id(void);

// releases a used process id to be reused
void release_process_id(int i);

// initializes the PID system 
void pid_bootstrap(void);

// cleanup the PID system 
void pid_cleanup(void);

// check if pid is in use
int pidUsed(int pid);

#endif
