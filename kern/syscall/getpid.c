#include <types.h>
#include <clock.h>
#include <copyinout.h>
#include <current.h>
#include <proc.h>
#include <syscall.h>



int32_t sys_getpid(){


    // TODO error handling! - nope, we don't get errors here ;]

    // just return the current thread's process' pid
    struct proc *parent_proc =  curthread->t_proc;
    int32_t cur_pid = (int32_t) parent_proc->PID;

    return cur_pid;
}
