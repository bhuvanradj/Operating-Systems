#ifndef PAGING_H
#define PAGING_H

#include "types.h"

#define V_MEM_BASE   0x000B8000
#define V_MEM_TERM_1 0x000B9000
#define V_MEM_TERM_2 0x000BA000
#define V_MEM_TERM_3 0x000BB000

/*	init_kernel_page
 *	DESCRIPTION: Initialize the 4MB kernel page by setting all the proper bits
 *	Inputs:	none
 *	Outputs: none
 *	Return value: none
 *	Side Effects:	Sets the values to initialize the 4MB kernel page
 */
extern void init_kernel_page();

/*	init_4k_page
 *	DESCRIPTION: Initialize the 4k kernel pages by setting all the proper bits, including for video memroy
 *	Inputs:	none
 *	Outputs: none
 *	Return value: none
 *	Side Effects:	Initializes video memory, set all other pages to not present
 */
extern void init_vidmem_pages();

/*	map_4m_page
 *	DESCRIPTION: Initialize a 4m page at the specified base address
 *	Inputs:	pd and base address of page to initialize
 *	Outputs: -1 if fail, 0 if success
 *	Return value: none
 *	Side Effects:
 */
extern uint32_t map_4m_page(uint32_t phys_base, uint32_t virt_base);

/*	map_4k_page
 *	DESCRIPTION: Initialize a 4k page at the specified base addresses
 *	Inputs:	pd and base address of page to initialize
 *	Outputs: -1 if fail, 0 if success
 *	Return value: none
 *	Side Effects: allocated a 4k page table
 */
extern uint32_t map_4k_page(uint32_t phys_base, uint32_t virt_base);


extern void remap_terminal_vidmap(int32_t terminal_id, uint32_t phys_base, uint32_t virt_base);
/*	disable_all_pages
 *	DESCRIPTION: Initialize all pages to not present
 *	Inputs:	none
 *	Outputs: none
 *	Return value: none
 *	Side Effects: Initialize all pages to not present
 */
extern void disable_all_pages();
/*	loadPageDirector
 *	DESCRIPTION: Enables paging by setting the correct bits in the cr0, cr3, and cr4 registers
 *	Inputs: none
 *	Outputs: none
 *	Return value: none
 *	Side Effects: enables paging by loading the page directory and setting bits
 */
 /* from: https://wiki.osdev.org/Pagingi
    and:  https://wiki.osdev.org/Setting_Up_Paging*/
extern void load_page_directory();
/*	reload_page_directory
 *	DESCRIPTION: reloads cr3 page directory
 *	Inputs: page directory pointer
 *	Outputs: none
 *	Return value: none
 *	Side Effects: flushes tlb
 */
extern void reload_page_directory();

extern int32_t check_permission(uint32_t virt_addr);

#endif
