/*
 * Copyright (c) 2000, 2001, 2002, 2003, 2004, 2005, 2008, 2009
 *	The President and Fellows of Harvard College.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <proc.h>
#include <addrspace.h>
#include <vm.h>
#include <coremap.h>
#include <diskmap.h>
#include <current.h>

/*
 * Note! If OPT_DUMBVM is set, as is the case until you start the VM
 * assignment, this file is not compiled or linked or in any way
 * used. The cheesy hack versions in dumbvm.c are used instead.
 */

struct addrspace *
as_create(void)
{
	struct addrspace *as;

	as = kmalloc(sizeof(struct addrspace));
	if (as == NULL) {
		return NULL;
	}
    //segment table is inside the struct
    //page table is a page
	as->page_table = (struct page_table_entry*)alloc_kpages(1);
    //ignore permissions should be zero


    as->heap_base = -1;
    as->heap_top = -1;
    as->heap_base_set = 0;
    
	return as;
}

int
as_copy(struct addrspace *old, struct addrspace **ret)
{
	struct addrspace *newas;

	newas = as_create();
	if (newas==NULL) {
		return ENOMEM;
	}

    //copy over segment table
	int i,j;
	for(i = 0; i < 4; i++) {
        if(old->segment_table[i].valid) {
            newas->segment_table[i].start = old->segment_table[i].start;
            newas->segment_table[i].end = old->segment_table[i].end;
            newas->segment_table[i].valid = 1;
            newas->segment_table[i].read = old->segment_table[i].read;
            newas->segment_table[i].write = old->segment_table[i].write;
            newas->segment_table[i].execute = old->segment_table[i].execute;
            newas->segment_table[i].index = old->segment_table[i].index;
            if(i == SG_DATA_BSS)
                set_heap_base(newas,old->segment_table[i].end);
        }
    }

    vaddr_t addr;
    
    //loop thru 1st lvl page table
    for(i = 0; i < 1024; i++)
        if(old->page_table[i].valid) {
            //if 1st lvl page table is valid, alloc_kpages a page for the newas' copy of the 2nd lvl page table
            addr = alloc_kpages(1);
            if(!addr) {
                //TODO set priority level
                return ENOMEM;
            }
            newas->page_table[i].index = addr >> 12;
            newas->page_table[i].valid = 1;
            //loop thru 2nd lvl page table
            for(j = 0; j < 1024; j++)
                if(((struct page_table_entry *)(old->page_table[i].index << 12))[j].valid) {
                    //if 2nd lvl page table is valid, alloc_kpages a page for the newas' copy of the page
                    addr = alloc_kpages(1);
                    if(!addr) {
                        //TODO: set priority level
                        return ENOMEM;
                    }
                    ((struct page_table_entry *)(newas->page_table[i].index << 12))[j].index = addr >> 12;
                    // if the page is on disk, load it in
                    if(((struct page_table_entry *)(old->page_table[i].index << 12))[j].on_disk) {
                        if(read_page(((struct page_table_entry *)(old->page_table[i].index << 12))[j].index, addr)) {
                            return EFAULT;
                        }            
                    } else {
                        //memcpy the page using kvaddrs
                        memcpy((void*)(addr), (void*)(((struct page_table_entry *)(old->page_table[i].index << 12))[j].index << 12), PAGE_SIZE);
                    }
                    //set page table on-disk bit to false
                    ((struct page_table_entry *)(newas->page_table[i].index << 12))[j].on_disk = 0;
                    //set page table valid
                    ((struct page_table_entry *)(newas->page_table[i].index << 12))[j].valid = 1;
                    //set coremap reverse lookup
                    set_lookup(get_page_index(addr),
                               (struct page_table_entry *)(newas->page_table[i].index << 12));
                    //unset coremap kernel bit
                    set_user_page(get_page_index(addr));
                }
        }

	*ret = newas;
	return 0;
}

void
as_destroy(struct addrspace *as)
{
    //loop thru 1st lvl page table
    int i,j;
    for(i = 0; i < 1024; i++)
        if(as->page_table[i].valid) {
            //if 1st lvl page table entry is valid, loop thru 2nd lvl page table entry
            for(j = 0; j < 1024; j++)
                if(((struct page_table_entry *)(as->page_table[i].index << 12))[j].valid)
                    //if 2nd lvl page table entry is valid, free that page
                    free_kpages(((struct page_table_entry *)(as->page_table[i].index << 12))[j].index << 12);
            //free 2nd lvl page table
            free_kpages(as->page_table[i].index << 12);
        }
    //free 1st level page table
    free_kpages((vaddr_t)as->page_table);
    //free addrspace struct
	kfree(as);
}

void
as_activate(void)
{
	struct addrspace *as;

	vm_tlbshootdown_all();

	as = proc_getas();
	if (as == NULL) {
		/*
		 * Kernel thread without an address space; leave the
		 * prior address space in place.
		 */
		return;
	}
}

void
as_deactivate(void)
{
	/*
	 * Write this. For many designs it won't need to actually do
	 * anything. See proc.c for an explanation of why it (might)
	 * be needed.
	 */
}


// sets the heap base depending on the end of the data/bss segment. called in as_define_region
void set_heap_base(struct addrspace *as, vaddr_t end_of_segment){

    // do the page alignment
    as->heap_base = ROUNDUP(end_of_segment, PAGE_SIZE);
    as->heap_top = as->heap_base;
    as->heap_base_set = 1;
    
}


void* sbrk__(intptr_t amt, int *err){

    struct addrspace* as = curthread->t_proc->p_addrspace;

    // lock the coremap
    acquire_cm_lock();


    // see if the heap_base was set
    if(as->heap_base_set == 0){
        release_cm_lock();
        return (void*) -1;
    }

    vaddr_t old_heap_top = as->heap_top;

    // calculate the new heap_top
    vaddr_t new_heap_top = as->heap_top + amt;

    // do some checking
    // * smaller than the heap_base?
    if(new_heap_top<as->heap_base){
        *err = EINVAL;
        release_cm_lock();
        return (void*) -1;
    }

    // * not page aligned
    vaddr_t rounded = ROUNDUP(new_heap_top, PAGE_SIZE);
    if(rounded != new_heap_top){
        // the amount would not be page aligned. change that.
        //new_heap_top = rounded;

        /*
        *err = EINVAL;
        release_cm_lock();
        return (void*) -1;   
        */
    }

    
    // * check if the new top would intersect with the stack segment

    if(as->segment_table[SG_STACK].start < new_heap_top){
        *err = ENOMEM;
        release_cm_lock();
        return (void*) -1;   
    }


    // everything is okay, set the new heap top
    as->heap_top = new_heap_top;

    // chang the segment size
    struct segment_table_entry* heap_segment = &as->segment_table[SG_DATA_BSS];
    heap_segment->end = new_heap_top;

    *err = 0;



    release_cm_lock();
    return (void *) old_heap_top;
}


/*
 * Set up a segment at virtual address VADDR of size MEMSIZE. The
 * segment in memory extends from VADDR up to (but not including)
 * VADDR+MEMSIZE.
 *
 * The READABLE, WRITEABLE, and EXECUTABLE flags are set if read,
 * write, or execute permission should be set on the segment. At the
 * moment, these are ignored. When you write the VM system, you may
 * want to implement them.
 */
int
as_define_region(struct addrspace *as, vaddr_t vaddr, size_t sz,
		 int readable, int writeable, int executable)
{
    unsigned int i;
    //readable and executable means code segment
    if(readable && !writeable && executable)
        i = SG_CODE;
    //just readable means read-only data segment
    else if(readable && !writeable && !executable)
        i = SG_STATIC_DATA;
    //readable and writeable means data/bss/heap segment
    else if(readable && writeable && !executable){
        i = SG_DATA_BSS;
        set_heap_base(as, vaddr+sz);
    //anything else is an error.
    }else
        return EINVAL;
    //set up that segment table entry
    as->segment_table[i].start = vaddr;
    as->segment_table[i].end = vaddr+sz;
    as->segment_table[i].valid = 1;
    as->segment_table[i].read = readable >> 2;
    as->segment_table[i].write = writeable >> 1;
    as->segment_table[i].execute = executable;
    as->segment_table[i].index = i;

    
	return 0;
}

int
as_prepare_load(struct addrspace *as)
{
    //set the ignore permissions bit for vm_faults in load_elf
	as->ignore_permissions = 1;
    return 0;
}

int
as_complete_load(struct addrspace *as)
{
    //unset ignore permissions bit
	as->ignore_permissions = 0;
	vm_tlbshootdown_all();
	return 0;
}

int
as_define_stack(struct addrspace *as, vaddr_t *stackptr)
{
	/* Initial user-level stack pointer */
	*stackptr = USERSTACK;
    //give read/write permissions to stack segment from USERSTACK-INITSTACKSIZE to USERSTACK
    as->segment_table[SG_STACK].start = *stackptr;
    as->segment_table[SG_STACK].end = *stackptr;
    as->segment_table[SG_STACK].valid = 1;
    as->segment_table[SG_STACK].read = 1;
    as->segment_table[SG_STACK].write = 1;
    as->segment_table[SG_STACK].execute = 0;
    as->segment_table[SG_STACK].index = SG_STACK;
	return 0;
}

