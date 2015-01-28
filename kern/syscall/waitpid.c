#include <types.h>
#include <kern/errno.h>
#include <kern/syscall.h>
#include <lib.h>
#include <mips/trapframe.h>
#include <proc.h>
#include <current.h>
#include <syscall.h>
#include <thread.h>
#include <pid.h>
#include <copyinout.h>

// function to compare pids
static
int
proc_comparator(void* left, void* right)
{
    struct proc *l = (struct proc*)left;
    int r = *(int*)right;
    return l->PID - r;
}

/*
--- waitpid
The parent wants to decrement the semaphore of the child process thread. 
The child process thread increments the semaphore if it exits. If the child 
has not finished (exit) yet, the parent will wait until the child exits. 
If the child already finished, parent don’t has to wait.
After a successfully wait (child process exits and parent process is up again 
and grabbed the return value of the child which is saved in the childs process struct)
Clean up child process ressources (free allocated space and pid)
Return child pid
*/
int sys_waitpid(int pid, int *status, int options, int *ret) {
//kprintf("DEAD THROUGH WAITPID!\n");
	struct thread* curt = curthread;
	//struct thread* childt;
	struct proc* curp = curt->t_proc;
	struct proc* childp;
	//int childreturn;
	//int result;

	// chek if pid argument named a nonexistent process
	/*
	if(pidUsed(pid) != 1)
	{
		return ESRCH;
	}
	*/

	// check if status is invalid
	if(status == NULL)
	{
		return EFAULT;
	}

	// check if option argument is valid
	if(options != 0)
	{
		return EINVAL;
	}

	lock_acquire(curp->p_childlist_lock);
	childp = (struct proc*)list_remove(curp->p_childlist, (void*) &pid, &proc_comparator);

	// check if pid argument named a process that was not a child of the current process
	if(childp == NULL)
	{
		lock_release(curp->p_childlist_lock);
		return ECHILD;
	}

	// wait until child process has exit
	P(childp->p_exit_sem_child);
	
/*

	childt = threadarray_get(&childp->p_threads, 0);
	if( childt == NULL)
	{
		lock_release(curp->p_childlist_lock);
		return -1;
	}


	result = thread_join(childt, &childreturn);
	if(result)
	{
		lock_release(curp->p_childlist_lock);
		return result;
	}
  */  
    //copy child pid now that child thread successfully exited
    	copyout(&childp->p_returnvalue, (userptr_t) status, sizeof(int));
	memcpy(ret, &pid, sizeof(int));
    
	V(childp->p_exit_sem_parent);

	lock_release(curp->p_childlist_lock);
	return 0;
}

