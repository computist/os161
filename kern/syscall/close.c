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
#include <limits.h>
#include <mips/trapframe.h>



int sys_close(struct trapframe *tf, int32_t *ret){

	// read the stuff from the trapframe
	
	// fd id
	int fd_id = (int) tf->tf_a0;

	// current file descriptor table
	struct fd_table* fdt = curthread->t_proc->p_fd_table;

	
	// see if the fd_id is valid
	if(!( fd_id > 0 && fd_id < __OPEN_MAX - 1 )){

		return EBADF;

	}

	// get the file descriptor for that file
	struct file_descriptor* fd =  get_fd(fdt, fd_id);

	// see if we have this ID
	if(fd == NULL){
		return EBADF;
	}

	// otherwise, close it
	int res = fd_close(fdt, fd);

	if(res){
		return res;
	}

	// set the fd ID
	*ret = 0;

	// and we are done
	return 0;
}



