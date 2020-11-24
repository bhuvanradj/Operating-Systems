#include "paging.h"
#include "x86_desc.h"
#include "lib.h"
#include "types.h"
#include "tasks.h"

#define PAGE_TABLE_SIZE 1024
#define KERNEL_PHYS_ADDR 0x400000
#define M_OFFSET 22
#define K_OFFSET 12
#define TEN_BIT_MASK 0x03FF

/* Page directory entries (Goes in Page Directory)*/
typedef union pde_4k_desc_t {
    uint32_t val;
    struct {
        uint32_t p          : 1;
        uint32_t rw         : 1;
        uint32_t us         : 1;
        uint32_t pwt        : 1;
        uint32_t pcd        : 1;
        uint32_t a          : 1;
        uint32_t reserved0  : 1;
        uint32_t ps         : 1;
        uint32_t g          : 1;
        uint32_t avail      : 3;
        uint32_t page_table_base_address : 20;
    } __attribute__ ((packed));
} pde_4k_desc_t;

typedef union pde_4m_desc_t {
    uint32_t val;
    struct {
        uint32_t p          : 1;
        uint32_t rw         : 1;
        uint32_t us         : 1;
        uint32_t pwt        : 1;
        uint32_t pcd        : 1;
        uint32_t a          : 1;
        uint32_t d          : 1;
        uint32_t ps         : 1;
        uint32_t g          : 1;
        uint32_t avail      : 3;
        uint32_t pat        : 1;
        uint32_t reserved0  : 9;
        uint32_t page_base_address : 10;
    } __attribute__ ((packed));
} pde_4m_desc_t;

typedef union pde_desc_t {
    pde_4k_desc_t k_type;
    pde_4m_desc_t m_type;
} pde_desc_t;

/* Page table entries (Goes in Page Tables)*/
typedef union pte_desc_t {
    uint32_t val;
    struct {
        uint32_t p          : 1;
        uint32_t rw         : 1;
        uint32_t us         : 1;
        uint32_t pwt        : 1;
        uint32_t pcd        : 1;
        uint32_t a          : 1;
        uint32_t d          : 1;
        uint32_t pat        : 1;
        uint32_t g          : 1;
        uint32_t avail      : 3;
        uint32_t page_base_address : 20;
    } __attribute__ ((packed));
} pte_desc_t;

/* pd_0 as initial kernel page, pd_1-6 for user programs */
pde_desc_t pd_arr[7][PAGE_TABLE_SIZE] __attribute__((aligned (4096)));
pte_desc_t pt_arr[7][PAGE_TABLE_SIZE] __attribute__((aligned (4096)));

/* dynamically allocated page tables */
pte_desc_t pt_vidmap[3][PAGE_TABLE_SIZE] __attribute__((aligned (4096)));

#define pd (pd_arr[current_task_id])
#define pt (pt_arr[current_task_id])


/*	init_kernel_page
 *	DESCRIPTION: Initialize the 4MB kernel page by setting all the proper bits
 *	Inputs:	none
 *	Outputs: none
 *	Return value: none
 *	Side Effects:	Sets the values to initialize the 4MB kernel page
 */
void init_kernel_page(){
	pd[KERNEL_PHYS_ADDR >> M_OFFSET].m_type.p = 1;
	pd[KERNEL_PHYS_ADDR >> M_OFFSET].m_type.rw = 1;
	pd[KERNEL_PHYS_ADDR >> M_OFFSET].m_type.us = 0;
	pd[KERNEL_PHYS_ADDR >> M_OFFSET].m_type.pwt = 0;
	pd[KERNEL_PHYS_ADDR >> M_OFFSET].m_type.pcd = 1;
	pd[KERNEL_PHYS_ADDR >> M_OFFSET].m_type.a = 0;
	pd[KERNEL_PHYS_ADDR >> M_OFFSET].m_type.d = 0;
	pd[KERNEL_PHYS_ADDR >> M_OFFSET].m_type.ps = 1;
	pd[KERNEL_PHYS_ADDR >> M_OFFSET].m_type.g = 1;
	pd[KERNEL_PHYS_ADDR >> M_OFFSET].m_type.avail = 0;
	pd[KERNEL_PHYS_ADDR >> M_OFFSET].m_type.pat = 0;
	pd[KERNEL_PHYS_ADDR >> M_OFFSET].m_type.reserved0 = 0;
	pd[KERNEL_PHYS_ADDR >> M_OFFSET].m_type.page_base_address = KERNEL_PHYS_ADDR >> M_OFFSET;
}
/*	init_4k_page
 *	DESCRIPTION: Initialize the 4k kernel pages by setting all the proper bits, including for video memroy
 *	Inputs:	none
 *	Outputs: none
 *	Return value: none
 *	Side Effects:	Initializes video memory, set all other pages to not present
 */
void init_vidmem_pages(){
  pd[V_MEM_BASE >> M_OFFSET].k_type.p = 1;
  pd[V_MEM_BASE >> M_OFFSET].k_type.rw = 1;
  pd[V_MEM_BASE >> M_OFFSET].k_type.us = 0;
  pd[V_MEM_BASE >> M_OFFSET].k_type.pwt = 0;
  pd[V_MEM_BASE >> M_OFFSET].k_type.pcd = 0; //*
  pd[V_MEM_BASE >> M_OFFSET].k_type.a = 0;
  pd[V_MEM_BASE >> M_OFFSET].k_type.reserved0 = 0;
  pd[V_MEM_BASE >> M_OFFSET].k_type.ps = 0;
  pd[V_MEM_BASE >> M_OFFSET].k_type.g = 1;
  pd[V_MEM_BASE >> M_OFFSET].k_type.avail = 0;
  pd[V_MEM_BASE >> M_OFFSET].k_type.page_table_base_address = ((uint32_t) pt) >> K_OFFSET;

  // Initialize video memory
  pt[V_MEM_BASE >> K_OFFSET].p = 1;
  pt[V_MEM_BASE >> K_OFFSET].rw = 1;
  pt[V_MEM_BASE >> K_OFFSET].us = 0;
  pt[V_MEM_BASE >> K_OFFSET].pwt = 0;
  pt[V_MEM_BASE >> K_OFFSET].pcd = 0;
  pt[V_MEM_BASE >> K_OFFSET].a = 0;
  pt[V_MEM_BASE >> K_OFFSET].d = 0;
  pt[V_MEM_BASE >> K_OFFSET].pat = 0;
  pt[V_MEM_BASE >> K_OFFSET].g = 1;
  pt[V_MEM_BASE >> K_OFFSET].avail = 0;
  pt[V_MEM_BASE >> K_OFFSET].page_base_address = V_MEM_BASE >> K_OFFSET;

  // Initialize 3 more 4k pages to store each of the terminals
  pt[V_MEM_TERM_1 >> K_OFFSET].val = pt[V_MEM_BASE >> K_OFFSET].val;
  pt[V_MEM_TERM_1 >> K_OFFSET].page_base_address = V_MEM_TERM_1 >> K_OFFSET;

  pt[V_MEM_TERM_2 >> K_OFFSET].val = pt[V_MEM_BASE >> K_OFFSET].val;
  pt[V_MEM_TERM_2 >> K_OFFSET].page_base_address = V_MEM_TERM_2 >> K_OFFSET;

  pt[V_MEM_TERM_3 >> K_OFFSET].val = pt[V_MEM_BASE >> K_OFFSET].val;
  pt[V_MEM_TERM_3 >> K_OFFSET].page_base_address = V_MEM_TERM_3 >> K_OFFSET;
}

/*	map_4m_page
 *	DESCRIPTION: Initialize the 4MB user program page by setting all the proper bits
 *	Inputs:	pd, phys_base that virt_base is mapped to
 *	Outputs: none
 *	Return value: none
 *	Side Effects:	Sets the values to initialize the 4MB user program page
 */
uint32_t map_4m_page(uint32_t phys_base, uint32_t virt_base) {
	// do not allow if this 4m page conflicts with kernel
	if (phys_base == KERNEL_PHYS_ADDR || virt_base == KERNEL_PHYS_ADDR) return -1;
	pd[virt_base >> M_OFFSET].m_type.p = 1;
	pd[virt_base >> M_OFFSET].m_type.rw = 1;
	pd[virt_base >> M_OFFSET].m_type.us = 1;
	pd[virt_base >> M_OFFSET].m_type.pwt = 0;
	pd[virt_base >> M_OFFSET].m_type.pcd = 1;
	pd[virt_base >> M_OFFSET].m_type.a = 0;
	pd[virt_base >> M_OFFSET].m_type.d = 0;
	pd[virt_base >> M_OFFSET].m_type.ps = 1;
	pd[virt_base >> M_OFFSET].m_type.g = 0;
	pd[virt_base >> M_OFFSET].m_type.avail = 0;
	pd[virt_base >> M_OFFSET].m_type.pat = 0;
	pd[virt_base >> M_OFFSET].m_type.reserved0 = 0;
	pd[virt_base >> M_OFFSET].m_type.page_base_address = phys_base >> M_OFFSET;
	return 0;
}

/*	map_4k_page
 *	DESCRIPTION: Initialize the 4kB user page by setting all the proper bits
 *	Inputs:	pd, phys_base that virt_base is mapped to
 *	Outputs: none
 *	Return value: none
 *	Side Effects:	Sets the values to initialize the 4kB user program page
 */
uint32_t map_4k_page(uint32_t phys_base, uint32_t virt_base) {
    int32_t terminal_id = current_task_pcb->terminal_id;
	/* get upper 10 bits of virt */
	pd[virt_base >> M_OFFSET].k_type.p = 1;
	pd[virt_base >> M_OFFSET].k_type.rw = 1;
	pd[virt_base >> M_OFFSET].k_type.us = 1;
	pd[virt_base >> M_OFFSET].k_type.pwt = 0;
	pd[virt_base >> M_OFFSET].k_type.pcd = 0;
	pd[virt_base >> M_OFFSET].k_type.a = 0;
	pd[virt_base >> M_OFFSET].k_type.reserved0 = 0;
	pd[virt_base >> M_OFFSET].k_type.ps = 0;
	pd[virt_base >> M_OFFSET].k_type.g = 0;
	pd[virt_base >> M_OFFSET].k_type.avail = 0;
	pd[virt_base >> M_OFFSET].k_type.page_table_base_address = ((uint32_t) &pt_vidmap[terminal_id]) >> K_OFFSET;

	/* wipe page table */
	int j;
	for (j = 0; j < PAGE_TABLE_SIZE; j++) {
		pt_vidmap[terminal_id][j].p = 0;
	}
	/* get the middle 10 bits of virt, mask by 3FF to only get 10 bits*/
	pt_vidmap[terminal_id][(virt_base >> K_OFFSET) & TEN_BIT_MASK].p = 1;
	pt_vidmap[terminal_id][(virt_base >> K_OFFSET) & TEN_BIT_MASK].rw = 1;
	pt_vidmap[terminal_id][(virt_base >> K_OFFSET) & TEN_BIT_MASK].us = 1;
	pt_vidmap[terminal_id][(virt_base >> K_OFFSET) & TEN_BIT_MASK].pwt = 0;
	pt_vidmap[terminal_id][(virt_base >> K_OFFSET) & TEN_BIT_MASK].pcd = 0;
	pt_vidmap[terminal_id][(virt_base >> K_OFFSET) & TEN_BIT_MASK].a = 0;
	pt_vidmap[terminal_id][(virt_base >> K_OFFSET) & TEN_BIT_MASK].d = 0;
	pt_vidmap[terminal_id][(virt_base >> K_OFFSET) & TEN_BIT_MASK].pat = 0;
	pt_vidmap[terminal_id][(virt_base >> K_OFFSET) & TEN_BIT_MASK].g = 0;
	pt_vidmap[terminal_id][(virt_base >> K_OFFSET) & TEN_BIT_MASK].avail = 0;
	pt_vidmap[terminal_id][(virt_base >> K_OFFSET) & TEN_BIT_MASK].page_base_address = phys_base >> K_OFFSET;

	return 0;
}

/*	remap_terminal_vidmap
 *	DESCRIPTION: remaps vidmaped pages
 *	Inputs:	terminalid pd, phys_base that virt_base is mapped to
 *	Outputs: none
 *	Return value: none
 *	Side Effects:
 */
void remap_terminal_vidmap(int32_t terminal_id, uint32_t phys_base, uint32_t virt_base) {
    pt_vidmap[terminal_id][(virt_base >> K_OFFSET) & TEN_BIT_MASK].page_base_address = phys_base >> K_OFFSET;
}

/*	disable_all_pages
 *	DESCRIPTION: Initialize all pages to not present
 *	Inputs:	none
 *	Outputs: none
 *	Return value: none
 *	Side Effects: Initialize all pages to not present
 */
void disable_all_pages() {
	int i;
	for (i = 0; i < PAGE_TABLE_SIZE; i++) {
		pd[i].k_type.p = 0;
		pt[i].p = 0;
	}
}

/*	check_permission
 *	DESCRIPTION: checks permission of a virtual address
 *	Inputs:	none
 *	Outputs: none
 *	Return value: -1 if page not present, or return US level
 */
int32_t check_permission(uint32_t virt_addr) {
	if (pd[virt_addr >> M_OFFSET].m_type.p) {
		if (pd[virt_addr >> M_OFFSET].m_type.ps) return (int32_t) pd[virt_addr >> M_OFFSET].m_type.us;
		pte_desc_t* pt_temp = (pte_desc_t*) ((pd[virt_addr >> M_OFFSET].k_type.page_table_base_address) << K_OFFSET);
		if (pt_temp[(virt_addr >> K_OFFSET) & TEN_BIT_MASK].p) return (int32_t) pt_temp[(virt_addr >> K_OFFSET) & TEN_BIT_MASK].us;
		return -1;
	}
	else return -1;
}

/*	loadPageDirector
 *	DESCRIPTION: Enables paging by setting the correct bits in the cr0, cr3, and cr4 registers
 *	Inputs: none
 *	Outputs: none
 *	Return value: none
 *	Side Effects: enables paging by loading the page directory and setting bits
 */
 /* from: https://wiki.osdev.org/Paging
    and:  https://wiki.osdev.org/Setting_Up_Paging*/

void load_page_directory() {
	// load page directory in cr3
	// enable 4m pages in cr4
	// enable paging in cr0
	asm volatile("			\n\
	movl %0, %%eax 			\n\
    movl %%eax, %%cr3   	\n\
    movl %%cr4, %%eax		\n\
    orl $0x00000010, %%eax  \n\
    movl %%eax, %%cr4		\n\
    movl %%cr0, %%eax		\n\
    orl $0x80000001, %%eax	\n\
    movl %%eax, %%cr0		\n\
	"
	:
	: "r" ((uint32_t)pd)
	);
}
/*	reload_page_directory
 *	DESCRIPTION: reloads cr3 page directory
 *	Inputs: page directory pointer
 *	Outputs: none
 *	Return value: none
 *	Side Effects: flushes tlb
 */
void reload_page_directory() {
	// reload page directory in cr3
	asm volatile ("			\n\
	movl %0, %%eax 			\n\
    movl %%eax, %%cr3   	\n\
	"
	:
	: "r" ((uint32_t)pd)
	);
}
