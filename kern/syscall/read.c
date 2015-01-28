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




int sys_read(struct trapframe *tf, int32_t *ret){

	// read the stuff from the trapframe
	
	// fd id
	int fd_id = (int) tf->tf_a0;

	// pointer to the buffer (in user space?)
	userptr_t user_buffer = (userptr_t) tf->tf_a1;

	// buffer length
	size_t buf_size = (size_t) tf->tf_a2;

	// current file descriptor table
	struct fd_table* fdt = curthread->t_proc->p_fd_table;



	// see if the fd_id is valid
	if(!( fd_id >= 0 && fd_id < __OPEN_MAX  )){

		return EBADF;

	}

	
	// get the file descriptor for that file
	struct file_descriptor* fd =  get_fd(fdt, fd_id);

	// see if we have this ID
	if(fd == NULL){
		return EBADF;
	}


	// create the kernel buffer
	//char kbuf[buf_size];
	size_t read_bytes = 0;


	int res = fd_read(fd, user_buffer, buf_size, &read_bytes);


	if(res){
		return res;
	}

	// copy into user space
	//res = copyout(kbuf, user_buffer, buf_size);
	//if (res) {
	//	return res;
	//}



	// set the number of bytes read
	*ret = read_bytes; // TODO CAST?

	// and we are done
	return 0;
}





