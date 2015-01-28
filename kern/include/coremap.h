#ifndef _H_COREMAP_
#define _H_COREMAP_

#define BOOKKEEPING

#include <types.h>
#include <addrspace.h>

struct dummy_table_entry{
	int test;
};

struct cm_entry{

    int free               				;  // indicates if the page is free
    int kernel             				;  // indicates if the page is a kernel page
    struct page_table_entry *pte;   	 // pointer to the page table entry
    //unsigned int                    : 30; // this is a huge waste of space. refactor at some point

};

struct empty_struct{};

// acquires / releases the coremap lock
void acquire_cm_lock(void);
void release_cm_lock(void);

// performs a selftest on the coremap
void coremap_selftest(void);

// returns the virtual (kernel) address of the page with the given address
vaddr_t get_page_vaddr(unsigned int page_index);

// returns the page index of the page with the given virtual (kernel) address 
unsigned int get_page_index(vaddr_t p_kaddr);

// sets up the space in virtual memory to hold the coremap
void coremap_bootstrap(void);

// returns true if the page in physical memory is not occupied, false otherwise
bool is_free(unsigned int page_index);

// sets the specified page to occupied
void set_occupied(unsigned int page_index);

// sets the specified page to free and writes deadbeef
void set_free(unsigned int page_index);

// checks if the speciefied page is a kernel page
bool is_kernel_page(unsigned int page_index);

// sets the specified page as a kernel page
void set_kernel_page(unsigned int page_index);

// sets the specified page as a user page
void set_user_page(unsigned int page_index);

// get the index of a free page. returns false if RAM full
bool get_free_page(unsigned int* page_index);

// set the reverse lookup entry of the page
void set_lookup(unsigned int page_index, struct page_table_entry * pte);

// returns the number of pages available
unsigned int get_coremap_size(void);

// returns a swappable page. a swapable page is every page which is not a kernel page and occupied
bool get_swappable_page(unsigned int* page_index);



// book keeping (cbk - coremap book keeping)
unsigned int cbk_pages_allocated;
unsigned int cbk_pages_freed;
unsigned int cbk_pages_in_use;
unsigned int cbk_pages_free;
struct vnode* swap_disk;





#endif

