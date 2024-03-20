/**
 * ideal_indirection
 * CS 341 - Spring 2024
 */
#include "mmu.h"
#include <assert.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
/*
tlb is a singly linked list with virt addr as key and phy addr as val
    tlb_flush() in tlb.h 
    - Clear tlb data
    tlb_add_pte() in tlb.h
    - add node to SLL, removes LRU If cap
    tlb_get_pte() in tlb.h
    - gets node of virt addr, update LRU and return NULL if not found

mmu struct of 2 arrays for page direcs + segs for each proc, also has tlb
    mmu_raise_segmentation_fault() in mmu.h
    - raises seg fault, proc tries invalid or illegal mem addr, not in any seg or no perms
    mmu_tlb_miss() in mmu.h
    - raises tlb miss
    mmu_raise_page_fault() in mmu.h
    - raise page fault, use when proc tries access to page not mapped on phy mem

kernel char array for phy mem that is very large, has mapping for table entry to page
    ask_kernel_for_frame() in kernel.h
    - returns phy addr for an entry, like malloc
    get_system_pointer_from_pde() in kernel.h
    - conv base addr of page table pointed by page direc entry into addr for machine
    read_page_from_disk() in kernel.h
    - read page of phy mem on disk
    get_system_pointer_from_address() in kernel.h
    - conv addr of sim phy mem to addr used by machine

segments, struct with start and end addr of a segment in virt mem, perms and grow direc
    find_segment() in segments.h
    - find segment in virt mem space that contains given addr
    address_in_segmentations() in segments.h
    - check if addr exists within segment of virt mem
*/
mmu *mmu_create() {
    mmu *my_mmu = calloc(1, sizeof(mmu));
    my_mmu->tlb = tlb_create();
    return my_mmu;
}

void mmu_read_from_virtual_address(mmu *this, addr32 virtual_address,
                                   size_t pid, void *buffer, size_t num_bytes) {
    assert(this);
    assert(pid < MAX_PROCESS_ID);
    assert(num_bytes + (virtual_address % PAGE_SIZE) <= PAGE_SIZE);
    // TODO: Implement me!
    // setting up local vars
    tlb **tlb = &this->tlb;
    page_directory *page_direc = this->page_directories[pid];
    vm_segmentations *segs = this->segmentations[pid];
    // decomposing virtual addr
    uint32_t page_direc_idx = virtual_address >> 22;
    uint32_t page_table_idx = (virtual_address >> NUM_OFFSET_BITS) & 0x3FF;
    uint32_t page_offset = virtual_address & 0xFFF;

    // Use pid to check for a context switch. If there was a switch, flush the TLB
    bool is_switch = pid != this->curr_pid;
    if (is_switch) {
        tlb_flush(tlb);
        this->curr_pid = pid;
    }
    // Make sure that the address is in one of the segmentations. If not, raise a segfault and return
    if (!address_in_segmentations(segs, virtual_address)) {
        mmu_raise_segmentation_fault(this);
        return;
    }
    // Check has proper perms, not needed since there is only R/W and R. TODO for write
    vm_segmentation *curr_seg = find_segment(segs, virtual_address);
    addr32 perms = curr_seg->permissions;
    if (!(perms & READ)) { // READ comes from segment.h
        mmu_raise_segmentation_fault(this);
        return;
    }
 
    // Check the TLB for the page table entry. If it’s not there:
    page_table_entry *tlb_node_PTE = tlb_get_pte(tlb, virtual_address & 0xFFFFF000);
    if (tlb_node_PTE == NULL) {
        //     Raise a TLB miss
        mmu_tlb_miss(this);
        //     Get the page directory entry. If it’s not present in memory:
        page_directory_entry *page_direc_entry = &page_direc->entries[page_direc_idx];
        if (page_direc_entry->present == 0) {
            //    Raise a page fault
            mmu_raise_page_fault(this);
            //    Ask the kernel for a frame
            addr32 frame_addr = ask_kernel_for_frame(NULL);
            // Update the page directory entry’s present, read_write, and user_supervisor flags
            read_page_from_disk(tlb_node_PTE);
            page_direc_entry->base_addr = frame_addr >> NUM_OFFSET_BITS;
            page_direc_entry->present = 1;
            page_direc_entry->read_write = 0; // MAY NEED TO CHANGE
            page_direc_entry->user_supervisor = 1;
        }
        //     Get the page table using the PDE
        page_table *p_table = (page_table *) get_system_pointer_from_pde(page_direc_entry);
        //     Get the page table entry from the page table. Add the entry to the TLB
        tlb_node_PTE = &(p_table->entries[page_table_idx]);
    }
    // If the page table entry is not present in memory:
    if (tlb_node_PTE && tlb_node_PTE->present == 0) {
        //     Raise a page fault
        mmu_raise_page_fault(this);
        //     Ask the kernel for a frame
        tlb_node_PTE->base_addr = (ask_kernel_for_frame(tlb_node_PTE) >> NUM_OFFSET_BITS);
        read_page_from_disk(tlb_node_PTE);
        //     Update the page table entry’s present, read_write, and user_supervisor flags
        tlb_node_PTE->present = 1;
        tlb_node_PTE->read_write = 0;
        tlb_node_PTE->user_supervisor = 1;
    }
    // Read the page from disk
    // Check that the user has permission to perform the read or write operation. If not, raise a segfault and return
    // Use the page table entry’s base address and the offset of the virtual address to compute the physical address. Get a physical pointer from this address
    // Perform the read or write operation
    // Mark the PTE as accessed. If writing, also mark it as dirty.
    tlb_node_PTE->accessed = 1;
    void *mac_addr = get_system_pointer_from_pte(tlb_node_PTE) + page_offset;
    memcpy(buffer, mac_addr, num_bytes);
    tlb_add_pte(tlb, virtual_address & 0xFFFFF000, tlb_node_PTE);
}


void mmu_write_to_virtual_address(mmu *this, addr32 virtual_address, size_t pid,
                                  const void *buffer, size_t num_bytes) {
    assert(this);
    assert(pid < MAX_PROCESS_ID);
    assert(num_bytes + (virtual_address % PAGE_SIZE) <= PAGE_SIZE);
    // TODO: Implement me!
    // setting up local vars
    tlb **tlb = &this->tlb;
    page_directory *page_direc = this->page_directories[pid];
    vm_segmentations *segs = this->segmentations[pid];
    // decomposing virtual addr
    uint32_t page_direc_idx = virtual_address >> 22;
    uint32_t page_table_idx = (virtual_address >> NUM_OFFSET_BITS) & 0x3FF;
    uint32_t page_offset = virtual_address & 0xFFF;

    // Use pid to check for a context switch. If there was a switch, flush the TLB
    bool is_switch = pid != this->curr_pid;
    if (is_switch) {
        tlb_flush(tlb);
        this->curr_pid = pid;
    }
    // Make sure that the address is in one of the segmentations. If not, raise a segfault and return
    if (!address_in_segmentations(segs, virtual_address)) {
        mmu_raise_segmentation_fault(this);
        return;
    }
    // Check has proper perms, not needed since there is only R/W and R. TODO for write
    vm_segmentation *curr_seg = find_segment(segs, virtual_address);
    addr32 perms = curr_seg->permissions;
    if (!(perms & WRITE)) { // READ comes from segment.h
        mmu_raise_segmentation_fault(this);
        return;
    }
 
    // Check the TLB for the page table entry. If it’s not there:
    page_table_entry *tlb_node_PTE = tlb_get_pte(tlb, virtual_address & 0xFFFFF000);
    if (tlb_node_PTE == NULL) {
        //     Raise a TLB miss
        mmu_tlb_miss(this);
        //     Get the page directory entry. If it’s not present in memory:
        page_directory_entry *page_direc_entry = &page_direc->entries[page_direc_idx];
        if (page_direc_entry->present == 0) {
            //    Raise a page fault
            mmu_raise_page_fault(this);
            //    Ask the kernel for a frame
            addr32 frame_addr = ask_kernel_for_frame(NULL);
            // Update the page directory entry’s present, read_write, and user_supervisor flags
            page_direc_entry->base_addr = frame_addr >> NUM_OFFSET_BITS;
            read_page_from_disk(tlb_node_PTE);
            page_direc_entry->present = 1;
            page_direc_entry->read_write = 1; // MAY NEED TO CHANGE
            page_direc_entry->user_supervisor = 1;
        }
        //     Get the page table using the PDE
        page_table *p_table = (page_table *) get_system_pointer_from_pde(page_direc_entry);
        //     Get the page table entry from the page table. Add the entry to the TLB
        tlb_node_PTE = &(p_table->entries[page_table_idx]);
    }
    // If the page table entry is not present in memory:
    if (tlb_node_PTE && tlb_node_PTE->present == 0) {
        //     Raise a page fault
        mmu_raise_page_fault(this);
        //     Ask the kernel for a frame
        tlb_node_PTE->base_addr = (ask_kernel_for_frame(tlb_node_PTE) >> NUM_OFFSET_BITS);
        //     Update the page table entry’s present, read_write, and user_supervisor flags
        read_page_from_disk(tlb_node_PTE);
        tlb_node_PTE->present = 1;
        tlb_node_PTE->read_write = 1;
        tlb_node_PTE->user_supervisor = 1;
    }
    // Read the page from disk
    // Check that the user has permission to perform the read or write operation. If not, raise a segfault and return
    // Use the page table entry’s base address and the offset of the virtual address to compute the physical address. Get a physical pointer from this address
    // Perform the read or write operation
    // Mark the PTE as accessed. If writing, also mark it as dirty.
    tlb_node_PTE->accessed = 1;
    tlb_node_PTE->dirty = 1;
    void *mac_addr = get_system_pointer_from_pte(tlb_node_PTE) + page_offset;
    memcpy(mac_addr, buffer, num_bytes);
    tlb_add_pte(tlb, virtual_address & 0xFFFFF000, tlb_node_PTE);
}

void mmu_tlb_miss(mmu *this) {
    this->num_tlb_misses++;
}

void mmu_raise_page_fault(mmu *this) {
    this->num_page_faults++;
}

void mmu_raise_segmentation_fault(mmu *this) {
    this->num_segmentation_faults++;
}

void mmu_add_process(mmu *this, size_t pid) {
    assert(pid < MAX_PROCESS_ID);
    addr32 page_directory_address = ask_kernel_for_frame(NULL);
    this->page_directories[pid] =
        (page_directory *)get_system_pointer_from_address(
            page_directory_address);
    page_directory *pd = this->page_directories[pid];
    this->segmentations[pid] = calloc(1, sizeof(vm_segmentations));
    vm_segmentations *segmentations = this->segmentations[pid];

    // Note you can see this information in a memory map by using
    // cat /proc/self/maps
    segmentations->segments[STACK] =
        (vm_segmentation){.start = 0xBFFFE000,
                          .end = 0xC07FE000, // 8mb stack
                          .permissions = READ | WRITE,
                          .grows_down = true};

    segmentations->segments[MMAP] =
        (vm_segmentation){.start = 0xC07FE000,
                          .end = 0xC07FE000,
                          // making this writeable to simplify the next lab.
                          // todo make this not writeable by default
                          .permissions = READ | EXEC | WRITE,
                          .grows_down = true};

    segmentations->segments[HEAP] =
        (vm_segmentation){.start = 0x08072000,
                          .end = 0x08072000,
                          .permissions = READ | WRITE,
                          .grows_down = false};

    segmentations->segments[BSS] =
        (vm_segmentation){.start = 0x0805A000,
                          .end = 0x08072000,
                          .permissions = READ | WRITE,
                          .grows_down = false};

    segmentations->segments[DATA] =
        (vm_segmentation){.start = 0x08052000,
                          .end = 0x0805A000,
                          .permissions = READ | WRITE,
                          .grows_down = false};

    segmentations->segments[TEXT] =
        (vm_segmentation){.start = 0x08048000,
                          .end = 0x08052000,
                          .permissions = READ | EXEC,
                          .grows_down = false};

    // creating a few mappings so we have something to play with (made up)
    // this segment is made up for testing purposes
    segmentations->segments[TESTING] =
        (vm_segmentation){.start = PAGE_SIZE,
                          .end = 3 * PAGE_SIZE,
                          .permissions = READ | WRITE,
                          .grows_down = false};
    // first 4 mb is bookkept by the first page directory entry
    page_directory_entry *pde = &(pd->entries[0]);
    // assigning it a page table and some basic permissions
    pde->base_addr = (ask_kernel_for_frame(NULL) >> NUM_OFFSET_BITS);
    pde->present = true;
    pde->read_write = true;
    pde->user_supervisor = true;

    // setting entries 1 and 2 (since each entry points to a 4kb page)
    // of the page table to point to our 8kb of testing memory defined earlier
    for (int i = 1; i < 3; i++) {
        page_table *pt = (page_table *)get_system_pointer_from_pde(pde);
        page_table_entry *pte = &(pt->entries[i]);
        pte->base_addr = (ask_kernel_for_frame(pte) >> NUM_OFFSET_BITS);
        pte->present = true;
        pte->read_write = true;
        pte->user_supervisor = true;
    }
}

void mmu_remove_process(mmu *this, size_t pid) {
    assert(pid < MAX_PROCESS_ID);
    // example of how to BFS through page table tree for those to read code.
    page_directory *pd = this->page_directories[pid];
    if (pd) {
        for (size_t vpn1 = 0; vpn1 < NUM_ENTRIES; vpn1++) {
            page_directory_entry *pde = &(pd->entries[vpn1]);
            if (pde->present) {
                page_table *pt = (page_table *)get_system_pointer_from_pde(pde);
                for (size_t vpn2 = 0; vpn2 < NUM_ENTRIES; vpn2++) {
                    page_table_entry *pte = &(pt->entries[vpn2]);
                    if (pte->present) {
                        void *frame = (void *)get_system_pointer_from_pte(pte);
                        return_frame_to_kernel(frame);
                    }
                    remove_swap_file(pte);
                }
                return_frame_to_kernel(pt);
            }
        }
        return_frame_to_kernel(pd);
    }

    this->page_directories[pid] = NULL;
    free(this->segmentations[pid]);
    this->segmentations[pid] = NULL;

    if (this->curr_pid == pid) {
        tlb_flush(&(this->tlb));
    }
}

void mmu_delete(mmu *this) {
    for (size_t pid = 0; pid < MAX_PROCESS_ID; pid++) {
        mmu_remove_process(this, pid);
    }

    tlb_delete(this->tlb);
    free(this);
    remove_swap_files();
}
