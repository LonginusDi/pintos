#include "vm/swap.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#define SECTOR_PER_PAGE (PGSIZE/BLOCK_SECTOR_SIZE)
static struct block * swap;
static char * free_swap;
static int swap_mapping_size;
static struct lock lock;
void swap_init() {
	swap = block_get_role(BLOCK_SWAP);
	if (swap == NULL)
		PANIC("No swap!!!");
	lock_init(&lock);
	swap_mapping_size = block_size(swap) / SECTOR_PER_PAGE / 8;
	free_swap = malloc(swap_mapping_size);
	memset(free_swap, 0, swap_mapping_size);
}

void swap_read(int page, void * uaddr) {
	//printf("read\n");
	lock_acquire(&lock);
	if ((free_swap[page / 8] & (1 << (page % 8))) == 0)
		PANIC("Not in swap");
	int i;
	for (i = 0; i < SECTOR_PER_PAGE; i++) {
		block_read(swap, (page * SECTOR_PER_PAGE) + i, uaddr + i * BLOCK_SECTOR_SIZE);
	}
	lock_release(&lock);
}

int swap_write(void * uaddr) {
	lock_acquire(&lock);
	int page = -1, i, j;
	//printf("write\n");
	for (i = 0; i < swap_mapping_size; i++) {
		bool found = false;
		for (j = 0; j < 8; j++) {
			if ((free_swap[i] & (1 << j)) == 0) {
				page = i * 8 + j;
				free_swap[i] |= (1 << j);
				found = true;
				break;
			}
		}
		if (found)
			break;
	}
	if (page == -1)
		PANIC("Not enough space");
	for (i = 0; i < SECTOR_PER_PAGE; i++) {
		//printf("page: %d\n", page);
		//printf("before block write: %d, %p\n", i, uaddr + i * BLOCK_SECTOR_SIZE);
		block_write(swap, page * SECTOR_PER_PAGE + i, uaddr + i * BLOCK_SECTOR_SIZE);
		//printf("after block write: %d\n", i);
	}
	lock_release(&lock);
	//printf("end write\n");
	return page;
}

void swap_remove(int page) {
	lock_acquire(&lock);
	uint8_t tmp = free_swap[page / 8] & 1 << (page % 8);
	if (tmp)
		free_swap[page / 8] ^= 1 << (page % 8);
	lock_release(&lock);
}