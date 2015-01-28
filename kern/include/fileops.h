#ifndef _FILE_OPS_H_
#define _FILE_OPS_H_


#include <types.h>
#include <addrspace.h>
#include <vnode.h>
#include <hashtable.h>
#include <list.h>
#include <synch.h>
#include <proc.h>
#include <limits.h>





// define the file descriptor struct
struct file_descriptor{
	char *filename;

	struct vnode *vnode;

	// O_RDONLY, etc...
	int flags;

	// the current offset of this filenode, this has to be set in the VNODE prior doing anything
	off_t offset;

	int index;

	// we may need this to ensure that we don't close a vnode although someone else is still using it
	int refcount;

	struct lock *fd_lock;

};


// creates a file descriptor. return null if not successfull
struct file_descriptor* fd_create(void);

// copies a file descriptor and increases the refcount
struct file_descriptor* fd_copy(struct file_descriptor* fd);


// destroys a given file descriptor
void fd_destroy(struct file_descriptor* fd);



//create the file table struct
struct fd_table{

	// the process of the file_table
	struct proc* proc;

	// dummy impl with indexes
	struct file_descriptor* fds[__OPEN_MAX];
	
	// the lock
	struct lock* lock;

	// the counter for file_descriptors
	int counter;

	// a queue of free file_descriptors TODO
	struct queue *fid_queue;
};


// creates a new file descriptor table for the given process - does not attach the table to the process
struct fd_table* fd_table_create(struct proc*);

struct fd_table* fd_table_copy(struct fd_table* fdt, struct proc* new_proc);

void fd_table_destroy(struct fd_table* fdt);

// get a new fd index, returns -1 if full
int get_new_fd_index(struct fd_table* fdt);

void release_fid(struct fd_table* fdt, int fid);

// add a file descriptor to the table
struct file_descriptor* add_file_descriptor(struct fd_table* fdt, char* filename, int flags);




/* open, read, write, close starts here. boom. */


//return an error code. the id of the file_descriptor can be retrieved via the file_descriptor pointer
int fd_open(struct fd_table* fdt, char* filename, int flags, int* fd_id);


int fd_read(struct file_descriptor* fd, userptr_t kbuf, size_t buflen, size_t* read_bytes);


int fd_write(struct file_descriptor* fd, userptr_t  kbuf, size_t buflen, size_t* written_bytes);


int fd_close(struct fd_table* fdt, struct file_descriptor* fd);


// returns the filedescriptor for a given id
struct file_descriptor* get_fd(struct fd_table* fdt, int fd_id);







#endif
