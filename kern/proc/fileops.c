#include "fileops.h"

#include <types.h>
#include <addrspace.h>
#include <vnode.h>
#include <lib.h>
#include <hashtable.h>
#include <list.h>
#include <kern/fcntl.h>
#include <kern/errno.h>
#include <vfs.h>
#include <uio.h>
#include <queue.h>


// creates a new file descriptor
struct file_descriptor* fd_create(void){

	struct file_descriptor *fd;

	// init the strcut
	fd = kmalloc(sizeof(struct file_descriptor));
	if(fd == NULL){
		return NULL;
	}

	// create the lock
	fd->fd_lock = lock_create("fd lock");
	if(fd->fd_lock == NULL){
		kfree(fd);
		return NULL;
	}

	fd->offset = 0;


	fd->refcount = 0;


	// the rest (mode, offset, and filename) will be set on call of fd_open()
	return fd;
};


// destroys a file descriptor
void fd_destroy(struct file_descriptor* fd){

	// destroy the lock
	lock_destroy(fd->fd_lock);

	// free the memory
	kfree(fd);
};


// copies a file_descriptor. vnode is newly opened
struct file_descriptor* fd_copy(struct file_descriptor* fd){

	// create a new file_descriptor
	struct file_descriptor* fd_copy = fd_create();

	// copy some fields
	fd_copy->filename = kstrdup(fd->filename);
	fd_copy->flags = fd->flags;
	fd_copy->offset = fd->offset;
	
	fd_copy->index = fd->index;
	fd_copy->refcount = 1; //legacy

	// we don't copy the vnode but rather create a new one
	// copy the filename because the stupid vfs_open does stupid thins
	char* filename = NULL;
	filename = kstrdup(fd->filename);
	int res = vfs_open(filename, fd_copy->flags, 0, &fd_copy->vnode);
	kfree(filename);

	KASSERT(res==0);

	return fd_copy;
}


// creates a new file descriptor table for the given process - does not attach the table to the process
struct fd_table* fd_table_create(struct proc* proc){

	struct fd_table *fdt;

	// init the strcut
	fdt = kmalloc(sizeof(struct fd_table));
	if(fdt == NULL){
		return NULL;
	}

	// create the lock
	fdt->lock = lock_create("file table lock");
	if(fdt->lock == NULL){
		kfree(fdt);
		return NULL;
	}

	// create the queue
	fdt->fid_queue = queue_create();
	if(fdt->fid_queue == NULL){
		lock_destroy(fdt->lock);
		kfree(fdt);
		return NULL;
	}

	
	// set the process
	fdt->proc = proc;

	// set counter to -1
	fdt->counter = -1;


	// initialize the fd table with NULL
	for(int i = 0; i < __OPEN_MAX; i++){
		fdt->fds[i] = NULL;
	}

	// add the file desriptors 0 and 1, or so.. - NOPE - this is done in proc.c


	// the rest (mode, offset, and filename) will be set on call of fd_open()
	return fdt;
};



struct fd_table* fd_table_copy(struct fd_table* fdt, struct proc* new_proc){

	// create a new file_table
	struct fd_table* fdt_copy = fd_table_create(new_proc);

	// copy the counter
	fdt_copy->counter = fdt->counter;

	// copy the active file descriptors
	for(int i = 0; i <= fdt->counter; i++){

		if(fdt->fds[i] != NULL){
			// there is a file descriptor, copy it
			fdt_copy->fds[i] = fd_copy(fdt->fds[i]);

			// if they are file descriptors for the console, reset the offset
			if(i<=2){
				fdt_copy->fds[i]->offset = 0;
			}
			
		}

	}

	// copy the queue
	// we do this by creating a temporary queue which is used to feed both old queues
	struct queue* tQ = queue_create();
	
	while(!queue_isempty(fdt->fid_queue)){

		queue_push(tQ, queue_front(fdt->fid_queue));
		queue_pop(fdt->fid_queue);

	}

	// now we the queues of both fdt's are empty. fill them again

	while(!queue_isempty(tQ)){

		int fid = (int) queue_front(tQ);

		queue_pop(tQ);

		queue_push(fdt->fid_queue, (void*) fid);
		queue_push(fdt_copy->fid_queue, (void*) fid);
	}

	// destroy the temporary Q
	queue_destroy(tQ);


	return fdt_copy;


}



void fd_table_destroy(struct fd_table* fdt){

	// destroy the lock
	lock_destroy(fdt->lock);

	// destroy the queue
	queue_destroy(fdt->fid_queue);


	// free the memory
	kfree(fdt);
}


// get a new fd index, returns -1 if full
int get_new_fd_index(struct fd_table* fdt){

	int new_id = -1;

	// see if we can recycle an ID (we do this first because otherwise we will get a huge queue of old IDs)
	if (!queue_isempty(fdt->fid_queue)){
		new_id = (int) queue_front(fdt->fid_queue);
		// remove that item from the queue
		queue_pop(fdt->fid_queue);

	}else{

		// otherwise, see if we can increment the PID counter
		if( fdt->counter < __OPEN_MAX){
			// we can increment
			 fdt->counter++;

			// return that new value
			new_id =  fdt->counter;
		} // else, we will just return new_id which should be -1
	}
	
	return new_id;

}


// releases a used fid
void release_fid(struct fd_table* fdt, int fid){
	queue_push(fdt->fid_queue, (void*) fid);	
}


// add a file descriptor to the table
struct file_descriptor* add_file_descriptor(struct fd_table* fdt, char* filename, int flags){
	KASSERT(fdt != NULL);

	// get a new file descriptor index
	int new_index = get_new_fd_index(fdt);

	if(new_index == -1){
		return NULL;
	}

	// create the fd struct
	struct file_descriptor* fd;
	fd = fd_create();

	// set the file name
	fd->filename = kstrdup(filename);

	// set the flags
	fd->flags = flags;

	// set the new id
	fd->index = new_index;

	// add the file descritpor to the tables 	
	fdt->fds[new_index] = fd;	

	return fd;
}


// creates a file descriptor, will return approriate error code or 0 on success
int fd_open(struct fd_table* fdt, char* filename, int flags, int* fd_index){

	// TODO, check if EFAULT	filename was an invalid pointer.


	

	// generate a vnode pointer
	struct vnode* mvnode;

	// copy the filename since vfs_open will fiddle around with it
	char* filename_copy =  NULL;
	filename_copy = kstrdup(filename);

	// try to open the vnode
	int res = vfs_open(filename_copy, flags, 0, &mvnode);
	kfree(filename_copy);

    //recopy string
    filename_copy = kstrdup(filename);
    
	// see if this worked
	if(res){
		return res;
	}

	// create the file descriptor
	struct file_descriptor* fd_temp = add_file_descriptor(fdt, filename_copy, flags);
	kfree(filename_copy);

	if(fd_temp==NULL){ // probably too many open files.
		vfs_close(mvnode);
		return EMFILE;
	}

	// return its id
	*fd_index = fd_temp->index;

	// add the vnode to the file descriptor
	fd_temp->vnode = mvnode;

	// add one reference
	fd_temp->refcount++;


	
	return 0;
}


                                       
int fd_read(struct file_descriptor* fd, userptr_t kbuf, size_t buflen, size_t* read_bytes){

	

	int res = 0;

	// lock
	lock_acquire(fd->fd_lock);

	

	struct iovec iov;
	struct uio io;

	
	off_t old_offset = fd->offset;

	// initialize iovec and uio for reading
	uio_uinit(&iov, &io, kbuf, buflen, fd->offset, UIO_READ); // this sets it to UIO_SYSSPACE, does it need to be UIO_USERSPACE? TODO


	// READ
	res = VOP_READ(fd->vnode, &io);

	
	// update offset in file descriptor
	fd->offset = io.uio_offset; // TODO CHECK ON FAILURE

	// set read bytes
	//*read_bytes = (size_t) ((unsigned int) io.uio_offset) - ((unsigned int) old_offset );
	*read_bytes = buflen - io.uio_resid;

	(void) old_offset;

	// free the stuff
	//kfree(&iov);
	//kfree(&io);

	lock_release(fd->fd_lock);
	return 0;
};



int fd_write(struct file_descriptor* fd, userptr_t  kbuf, size_t buflen, size_t* written_bytes){

	

	int res = 0;

	// lock
	lock_acquire(fd->fd_lock);	

	struct iovec iov;
	struct uio io;

	off_t old_offset = fd->offset;
	
	// initialize iovec and uio for reading
	uio_uinit(&iov, &io, kbuf, buflen, fd->offset, UIO_WRITE); // this sets it to UIO_SYSSPACE, does it need to be UIO_USERSPACE? TODO


	// READ
	res = VOP_WRITE(fd->vnode, &io);

	
	// update offset in file descriptor
	fd->offset = io.uio_offset; // TODO CHECK ON FAILURE

	// set written bytes
	//*written_bytes = (size_t) ((unsigned int) io.uio_offset) - ((unsigned int) old_offset );
	*written_bytes = buflen - io.uio_resid;

	(void) old_offset;

	
	lock_release(fd->fd_lock);
	return 0;
};


int fd_close(struct fd_table* fdt, struct file_descriptor* fd){

	(void) fdt;
	(void) fd;

	// decrease refcount
	fd->refcount--;

	// if refcount is 0 than it is save to close the vnode
	if(fd->refcount ==0){
		vfs_close(fd->vnode);

		// destroy the file descriptor after setting it to null in the table
		fdt->fds[fd->index] = NULL;
		release_fid(fdt, fd->index);

		fd_destroy(fd);
	}

	return 0;
}

struct file_descriptor* get_fd(struct fd_table* fdt, int fd_id){

	return fdt->fds[fd_id];
}



