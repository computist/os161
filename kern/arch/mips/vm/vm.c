
#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <mips/tlb.h>
#include <spl.h>
#include <vm.h>
#include <addrspace.h>
#include <current.h>
#include <proc.h>
#include <coremap.h>
#include <diskmap.h>

void
vm_bootstrap(void)
{
	//coremap_bootstrap();
}

/* Allocate/free some kernel-space virtual pages */
vaddr_t
alloc_kpages(int npages)
{
	acquire_cm_lock();

	vaddr_t page_addr = (vaddr_t)NULL;
	unsigned int  page_index;
	bool ret = false;

	// for simplicity we only allow to allocate one pace per call
	KASSERT(npages == 1);

	// get a free page
	ret = get_free_page(&page_index);
	// return null if no page is available or strange page_index received
	if (ret == false || page_index <= 0) {		
		release_cm_lock();
		return (vaddr_t)NULL;	
	}

 	// occupy page
    	set_occupied(page_index);

    	// Additional test
    	/*
    	// check if occupying worked
    	ret = is_free(page_index);
	if (ret) {		
		release_cm_lock();
		return NULL;
	}
	*/

	// Set kernel flag
    	set_kernel_page(page_index);

    	// Additional tests
    	/*
    	// check if kernel flag is set
    	ret = is_kernel_page(page_index);
	if (ret == false) {		
		release_cm_lock();
		return NULL;
	}
	*/

	// get address of page get_page_vaddr returns KVADDR
	page_addr = get_page_vaddr(page_index);

	release_cm_lock();

	return page_addr;
}

void
free_kpages(vaddr_t addr)
{
	acquire_cm_lock();
	int page_index = 0;
	bool ret = false;

	// check if vaddr_t is in kernel area
	if(addr < MIPS_KSEG0 || MIPS_KSEG1 <= addr) {
		release_cm_lock();
		KASSERT(false);
	}

	// translate KVADDR to VADDR -> happens in get_page_index()

	// get page index
	page_index = get_page_index(addr);


	// page should be in use
 	ret = is_free(page_index);
	if (ret == true) {		
		release_cm_lock();
		KASSERT(false);
	}

	// page should not be marked as kernel page
        /*
    ret = is_kernel_page(page_index);
	if (ret) {		
		release_cm_lock();
		KASSERT(false);
	}
        */
	// free page and deadbeef it
	set_free(page_index);


	release_cm_lock();
}

void
vm_tlbshootdown_all(void)
{
    int spl = splhigh();
    
    int i;
    for (i=0; i<NUM_TLB; i++) {
        tlb_write(TLBHI_INVALID(i), TLBLO_INVALID(), i);
    }

    splx(spl);
}

void
vm_tlbshootdown(const struct tlbshootdown *ts)
{
    (void)ts;
    vm_tlbshootdown_all();
}

int
vm_fault(int faulttype, vaddr_t faultaddress)
{
    int spl = splhigh();

    if (curproc == NULL) {
        /*
         * No process. This is probably a kernel fault early
         * in boot. Return EFAULT so as to panic instead of
         * getting into an infinite faulting loop.
         */
         splx(spl);
        return EFAULT;
    }

    struct addrspace * as = proc_getas();
    if (as == NULL) {
        /*
         * No address space set up. This is probably also a
         * kernel fault early in boot.
        */
        splx(spl);
        return EFAULT;
    }
    
    //loop through all segments and if the fault address is in that segment, check permissions
    //might need segment table lock?
    int i, found_segment = 0;
    for(i=0;i<4;i++) {
        if(faultaddress >= as->segment_table[i].start && faultaddress < as->segment_table[i].end) {
          found_segment = 1;
            if(!as->ignore_permissions) {
                switch (faulttype) {
                    //should probably throw an error somehow instead of kassert
                    case VM_FAULT_READONLY:
                        KASSERT(as->segment_table[i].write && as->page_table[faultaddress >> 22].valid && ((struct page_table_entry *)(as->page_table[faultaddress >> 22].index << 12))[(faultaddress >> 12)&1023].valid);
                        break;
                    case VM_FAULT_READ:
                        KASSERT(as->segment_table[i].read);
                        break;
                    case VM_FAULT_WRITE:
                        KASSERT(as->segment_table[i].write);
                        break;
                    default:
                        splx(spl);
                        return EINVAL;
                }
            }
        }
    }
    if(!found_segment) {
        //extend the stack if valid
        if(as->segment_table[SG_STACK].start - faultaddress < 10*PAGE_SIZE) {
            int pages = DIVROUNDUP(as->segment_table[SG_STACK].start - faultaddress, PAGE_SIZE);
            if(as->segment_table[SG_STACK].start - pages*PAGE_SIZE - as->segment_table[SG_DATA_BSS].end >= 10*PAGE_SIZE) {
                as->segment_table[SG_STACK].start -= pages*PAGE_SIZE;
            } else {
                splx(spl);
                return EFAULT;
            }
        }
    }
    
    //is the first level page table entry aka the second level page table valid?
    // to access the first level page table entry, you first get the index,
    // which is the first ten bits of faultaddress, so bitshift down 22.
    // you then use that to index into as->page-table to get the struct itself.
    if(!as->page_table[faultaddress >> 22].valid) {
        vaddr_t addr = alloc_kpages(1);
        if(!addr) {
            splx(spl);
            return EFAULT;
        }

        as->page_table[faultaddress >> 22].index = addr >> 12;
        as->page_table[faultaddress >> 22].valid = 1;
    }
    //is the second level page table entry aka the actual page valid?
    // to access the second level page table entry, you first access the second
    // level page table by bitshifting the first level page table entry up 12
    // to get the vkaddr.  You then cast it as a page_table_entry array, and then
    // access the index.  The index is the middle 10 bits of faultaddress, so you
    // bitshift down 12, and then mask out the upper 10 bits.
    if(!((struct page_table_entry *)(as->page_table[faultaddress >> 22].index << 12))[(faultaddress >> 12)&1023].valid ||
       ((struct page_table_entry *)(as->page_table[faultaddress >> 22].index << 12))[(faultaddress >> 12)&1023].on_disk) {
        //alloc a kpage
        vaddr_t addr = alloc_kpages(1);
        if(!addr) {
            splx(spl);
            return EFAULT;
        }
        // if the page is valid, but not in memory, load it in
        if(((struct page_table_entry *)(as->page_table[faultaddress >> 22].index << 12))[(faultaddress >> 12)&1023].valid  && ((struct page_table_entry *)(as->page_table[faultaddress >> 22].index << 12))[(faultaddress >> 12)&1023].on_disk) {
            if(read_page(((struct page_table_entry *)(as->page_table[faultaddress >> 22].index << 12))[(faultaddress >> 12)&1023].index, addr)) {
                splx(spl);
                return EFAULT;
            }
        }
        // set page table index
        ((struct page_table_entry *)(as->page_table[faultaddress >> 22].index << 12))[(faultaddress >> 12)&1023].index = addr >> 12;
        //set page table on-disk bit to false
        ((struct page_table_entry *)(as->page_table[faultaddress >> 22].index << 12))[(faultaddress >> 12)&1023].valid = 0;
        //set page table valid bit
        ((struct page_table_entry *)(as->page_table[faultaddress >> 22].index << 12))[(faultaddress >> 12)&1023].valid = 1;
        //set coremap reverse lookup
        set_lookup(get_page_index(addr),
                   (struct page_table_entry *)(as->page_table[faultaddress >> 22].index << 12));
        //unset coremap kernel bit
        set_user_page(get_page_index(addr));
    }
    
    //we are writing the vkaddr of the page itself, so we bitshift up 12 on
    //the second level page table entry's index
    //int spl = splhigh();
    unsigned int flags = TLBLO_VALID;
    switch (faulttype) {
        case VM_FAULT_READONLY:
            //replace the previous not dirty entry in the TLB with a dirtied one.
            i = tlb_probe(faultaddress & TLBHI_VPAGE, 0);
            KASSERT(i >= 0);
            tlb_write(faultaddress & TLBHI_VPAGE, KVADDR_TO_PADDR(((struct page_table_entry *)(as->page_table[faultaddress >> 22].index << 12))[(faultaddress >> 12)&1023].index << 12)|TLBLO_DIRTY|TLBLO_VALID, i);
            break;
        //read and write are the same except write sets the dirty bit
        case VM_FAULT_WRITE:
            //set the dirty bit
            flags = flags | TLBLO_DIRTY;
        case VM_FAULT_READ:
            //see if it is already in the tlb
            i = tlb_probe(faultaddress & TLBHI_VPAGE, 0);
            //if so, replace that entry, else replace a random one
            if(i >= 0)
                tlb_write(faultaddress & TLBHI_VPAGE, KVADDR_TO_PADDR(((struct page_table_entry *)(as->page_table[faultaddress >> 22].index << 12))[(faultaddress >> 12)&1023].index << 12)|flags, i);
            else
                tlb_random(faultaddress & TLBHI_VPAGE, KVADDR_TO_PADDR(((struct page_table_entry *)(as->page_table[faultaddress >> 22].index << 12))[(faultaddress >> 12)&1023].index << 12)|flags);
            break;
        default:
            splx(spl);
            return EINVAL;
    }
    splx(spl);
    return 0;
}

