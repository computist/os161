#include <types.h>
#include <copyinout.h>
#include <current.h>
#include <proc.h>
#include <fileops.h>
#include <syscall.h>
#include <addrspace.h>
#include <vnode.h>
#include <lib.h>
#include <hashtable.h>
#include <list.h>
#include <kern/fcntl.h>
#include <kern/errno.h>
#include <vfs.h>
#include <uio.h>
#include <mips/trapframe.h>



int sys_open(struct trapframe *tf, int32_t *ret){

	// read the stuff from the trapframe

	// filename
	char *filename = (char *)tf->tf_a0; // does this have to be -   const char *   - ?
	
	// flags
	int flags = (int) tf->tf_a1;

	// current file descriptor table
	struct fd_table* fdt = curthread->t_proc->p_fd_table;

	// now call the open function
	int fdi;

	int res = fd_open(fdt, filename, flags, &fdi);

	if(res){
		return res;
	}

	// set the fd ID
	*ret = fdi;

	// and we are done
	return 0;
}











