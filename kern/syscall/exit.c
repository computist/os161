#include <types.h>
#include <kern/errno.h>
#include <kern/syscall.h>
#include <lib.h>
#include <mips/trapframe.h>
#include <thread.h>
#include <proc.h>
#include <current.h>
#include <syscall.h>

/*
--- exit
Does process has child processes? (!empty(child_process_list))
if yes:
destroy all semaphores from the childs process threads (we don’t need them anymore)
set variable parent from all its childs structs to NULL
Clean up process ressources (free allocated space and pid) of all its child processes that are waiting that parent “joins”. 
Here we can also use the thread join mechanism again. 
The system call triggers the kernel to change the state of the waiting child process threads such that the cpu can cleanup the ressources.

Check if process has parent (parent != NULL)
if yes:
store exitcode in process struct (returnvalue = exitcode)
increment thread semaphore to maybe wake up the parent
process structure remains until parent “joins” or until it gets cleaned up
if no:
Clean up process ressources (free allocated space and pid)
*/
void sys_exit(int exitcode) {
//kprintf("DEAD THROUGH EXIT!\n");
	struct thread* curt = curthread;
	struct proc* curp = curt->t_proc;
	struct proc* childp = NULL;
	//struct thread* childt = NULL;

	// lock process struct
	//spinlock_acquire(&childp->p_lock);

	// remove join property of child processes
	lock_acquire(curp->p_childlist_lock);
	if(!list_isempty(curp->p_childlist)){ // if we have children
		while(!list_isempty(curp->p_childlist)) // as long as we have children
		{
			childp = (struct proc*)list_remove_front(curp->p_childlist);
			if(childp == NULL)				// TODO make shure that this not happens
			{
				//spinlock_release(&childp->p_lock);
				lock_release(curp->p_childlist_lock);
				KASSERT(false);
				// TODO die
			}

			//spinlock_acquire(&childp->p_lock);
			//childp->p_parent = NULL;

			//P(childp->p_exit_sem_child);
			// wait for parent
			V(childp->p_exit_sem_parent);
/*
			// if our child process still has children
			if(threadarray_num(&childp->p_threads)>0){


			childt = threadarray_get(&childp->p_threads, 0);
			if( childt == NULL)				// TODO make shure that this not happens
			{
				//spinlock_release(&childp->p_lock);
				lock_release(curp->p_childlist_lock);
				// TODO die
			}

			// update thread variables of child process
			childt->has_parent = false;
			childt->t_parent = NULL;
			//sem_destroy(childt->t_join_sem_child);
			//sem_destroy(childt->t_join_sem_parent);

			//spinlock_release(&childp->p_lock);
						
			}
*/

			curt->t_childs_to_join--;
					
		}
		
	}
	lock_release(curp->p_childlist_lock);

	

	// unlock process struct
	//spinlock_release(&childp->p_lock);
	
	curp->p_returnvalue = exitcode;


// TODO if smth breaks down in the join mechanism. Use another one here!!!!


/*
	// destroy process structure
    if(curp->p_parent != NULL) {
	//lock_acquire(curp->p_parent->p_childlist_lock);

	
	P(curp->p_exit_sem);

		proc_remthread(curt);
	//lock_release(curp->p_parent->p_childlist_lock);
	sem_destroy(curp->p_exit_sem);
		proc_destroy(curp);





    }else{
	proc_remthread(curt);
		proc_destroy(curp);

    }
	*/
if(curp->p_parent != NULL){
	V(curp->p_exit_sem_child);
	P(curp->p_exit_sem_parent);
} 

		proc_remthread(curt);
			// semaphores no longer needed
			sem_destroy(curp->p_exit_sem_child);
			sem_destroy(curp->p_exit_sem_parent);
		proc_destroy(curp);

	

//kprintf("ALIVE THROUGH EXIT?\n");

	// exit thread should cleanup the thread and release waiting parent processes
	thread_exit(exitcode);
	// should never happen since thread_exit() should not return
	panic("return from thread_exit in sys_exit\n");
}


