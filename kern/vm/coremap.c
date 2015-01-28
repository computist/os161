#include <coremap.h>
#include <types.h>
#include <lib.h>
#include <vm.h>
#include <spinlock.h>
#include <kern/fcntl.h>


// the borders of the ram after ram_bootstrap
paddr_t firstpaddr; // first address
paddr_t lastpaddr; // last address

struct cm_entry* coremap;

unsigned int number_of_pages_avail = -1;

// allocate a static spinlock
static struct spinlock coremap_lock;


// acquires the coremap lock
void acquire_cm_lock(void){
    spinlock_acquire(&coremap_lock);
}

// releases the coremap lock
void release_cm_lock(void){
    spinlock_release(&coremap_lock);
}

// sets up the space in virtual memory to hold the coremap
void coremap_bootstrap(void){


    // initialize book keeping if neccessary 
    #ifdef BOOKKEEPING
    cbk_pages_allocated = 0;
    cbk_pages_freed = 0;
    cbk_pages_in_use = 0;
    cbk_pages_free = 0;
    #endif


    // get the physical space which is available, we can not use steal mem at this point anymore
    ram_getsize(&firstpaddr, &lastpaddr);

    // get the number of bytes which will be available
    unsigned int number_of_bytes_avail = lastpaddr - firstpaddr; // I think that has to be +1.. TODO not critical  // Winfried: why do you think that? I mean when there are now bytes availavle, than last - first = 0, which is true, so why +1?

    // get the number of available pages
    number_of_pages_avail = number_of_bytes_avail / PAGE_SIZE;
    //number_of_bytes_avail--;

    #ifdef BOOKKEEPING
    cbk_pages_free = number_of_pages_avail;
    #endif



    /* testing struct sizes */
    size_t struct_size = sizeof(struct cm_entry);

    size_t struct_size2 = sizeof(struct empty_struct);

    size_t struct_size3 = sizeof(unsigned int);

    int page_size = PAGE_SIZE;
    (void) page_size;

    (void) struct_size2;

    (void) struct_size3;


    /* end testing struct sizes */

    // calculate the size of the coremap (in bytes)
    unsigned int coremap_size = struct_size * number_of_pages_avail;


    // calculate how much pages that would be, round up to be page aligned
    unsigned int number_of_pages =  DIVROUNDUP( coremap_size , PAGE_SIZE ); // TODO out of some reason that does not work, TODO still (feko)?




    KASSERT(number_of_pages > 0);


    

    // get a pointer for the coremap. we have to translate the physical address into 
    coremap = (struct cm_entry*) PADDR_TO_KVADDR(firstpaddr);

    // initialize the coremap
    for(unsigned int i = 0; i < number_of_pages_avail; i++){
        coremap[i].free = 1; 
        coremap[i].kernel = 0;
        coremap[i].pte = NULL;
    }

    // lock the pages which are occupied by the coremap
    for(unsigned int i = 0; i < number_of_pages; i++){
        coremap[i].free = 0; 
        coremap[i].kernel = 1;

        #ifdef BOOKKEEPING
        cbk_pages_free--;
        cbk_pages_in_use++;
        #endif

    }

    KASSERT(coremap[0].free == false);
    KASSERT(coremap[0].kernel == true);

    // now try to steal that space for the coremap
    // we do this by incrementing the firstpaddr pointer
    firstpaddr += number_of_pages * PAGE_SIZE;
    number_of_pages_avail -= number_of_pages;


    // and done.    
    kprintf("%u pages (%u bytest) occupied for the coremap\n", number_of_pages, coremap_size);   

    // intialize the spinlock
    spinlock_init(&coremap_lock);

    
    // and do a selftest    
    coremap_selftest();
    

    kprintf("coremap selftest passed \n");   

}

// returns a swappable page. a swapable page is every page which is not a kernel page and occupied
bool get_swappable_page(unsigned int* page_index){

    // get a random page index
    unsigned int r = random();

    unsigned int random_page_index = r % number_of_pages_avail;
    KASSERT(random_page_index >0 && random_page_index < number_of_pages_avail);

    // find a free page starting at the random_page_index
    unsigned int counter = 0;
    bool found_page = false;

    while(counter < number_of_pages_avail && !found_page){

        if(coremap[random_page_index].kernel == 0 && coremap[random_page_index].free == 0){
            found_page = true;
            break;
        }
        counter++;
        random_page_index++;
        random_page_index = random_page_index % number_of_pages_avail;
    }
    if(found_page)
        KASSERT(coremap[random_page_index].kernel == 0 && coremap[random_page_index].free == 0);

    *page_index = random_page_index;

    return found_page;
}


void coremap_selftest(void){

    acquire_cm_lock();

    KASSERT(coremap[0].free == false);
    KASSERT(coremap[0].kernel == true);

    // find the next free page
    unsigned int free_page_index;

    KASSERT(get_free_page(&free_page_index));

    KASSERT(free_page_index > 0);

    bool check = is_free(free_page_index);
    KASSERT(check);

    set_occupied(free_page_index);
    check = is_free(free_page_index);
    KASSERT(check == false);

    set_kernel_page(free_page_index);
    check = is_kernel_page(free_page_index);
    KASSERT(check);

    set_user_page(free_page_index);
    check = is_kernel_page(free_page_index);
    KASSERT(check == false);

    set_free(free_page_index);
    check = is_free(free_page_index);
    KASSERT(check);

    release_cm_lock();

}

bool get_free_page(unsigned int* page_index){

    for(unsigned int i = 0; i < number_of_pages_avail; i++){
        if(is_free(i)){
            *page_index = i;

            // get the kvaddr
            vaddr_t addr = get_page_vaddr(i);

            uint32_t* page = (uint32_t*) addr;
            //(void) page;

            // delete stuff
            for(unsigned int j = 0; j < PAGE_SIZE / sizeof(uint32_t); j++){
                page[j] = 0;
            }

            return true;
        }
    }

    *page_index = 0xBADEAFFE;
    return false;
}

// returns the virtual (kernel) address of the page with the given address
vaddr_t get_page_vaddr(unsigned int page_index){
    KASSERT(page_index < number_of_pages_avail);

    // calculate the first address of the desired page
    unsigned int p_addr = firstpaddr + (page_index * PAGE_SIZE);

    return (vaddr_t) PADDR_TO_KVADDR(p_addr);
}


// returns the page index of the page with the given virtual (kernel) address 
unsigned int get_page_index(vaddr_t p_kaddr){

    vaddr_t p_addr = KVADDR_TO_PADDR(p_kaddr);

    // page address has to be bigger than or equal the first page addr
    KASSERT(p_addr >= firstpaddr);

    // page addres has to be a page address
    KASSERT(((p_addr - firstpaddr) % PAGE_SIZE) == 0);

    // calculate page index
    unsigned int page_index = (p_addr - firstpaddr) / PAGE_SIZE;

    KASSERT(page_index < number_of_pages_avail);

    return page_index;
}



// returns true if the page in physical memory is not occupied, false otherwise
bool is_free(unsigned int page_index){ 
    KASSERT(page_index < number_of_pages_avail);
    
    return coremap[page_index].free;
}

// sets the specified page to occupied
void set_occupied(unsigned int page_index){ 
    KASSERT(page_index < number_of_pages_avail);
    
    coremap[page_index].free = false;    

    #ifdef BOOKKEEPING
    cbk_pages_free--;
    cbk_pages_in_use++;
    cbk_pages_allocated++;
    #endif

}

// sets the specified page to free and writes deadbeef
void set_free(unsigned int page_index){ 
    KASSERT(page_index < number_of_pages_avail);
 
    coremap[page_index].free = true;

    // get the kvaddr
    vaddr_t addr = get_page_vaddr(page_index);

    uint32_t* page = (uint32_t*) addr;

    // delete stuff
    for(unsigned int i = 0; i < PAGE_SIZE / sizeof(uint32_t); i++){
        page[i] = 0xDEADBEEF;
    }

    #ifdef BOOKKEEPING
    cbk_pages_free++;
    cbk_pages_in_use--;
    cbk_pages_freed++;
    #endif

}

// checks if the speciefied page is a kernel page
bool is_kernel_page(unsigned int page_index){ 
    KASSERT(page_index < number_of_pages_avail);
    
    return coremap[page_index].kernel;
}

// sets the specified page as a kernel page
void set_kernel_page(unsigned int page_index){ 
    KASSERT(page_index < number_of_pages_avail);

    coremap[page_index].kernel = 1;
}

// sets the specified page as a user page
void set_user_page(unsigned int page_index){ 
    KASSERT(page_index < number_of_pages_avail);
    
    coremap[page_index].kernel = 0;
}

//sets the lookup of the coremap entry
void set_lookup(unsigned int page_index, struct page_table_entry * pte) {
    KASSERT(page_index < number_of_pages_avail);
    
    coremap[page_index].pte = pte;
}

// returns the number of pages available
unsigned int get_coremap_size(void) {
    return number_of_pages_avail;
}


