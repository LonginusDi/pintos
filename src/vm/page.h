#ifndef VM_PAGE_H
#define VM_PAGE_H
#include "filesys/file.h"
#include "threads/thread.h"
#include <list.h>
enum vm_page_type {
	VM_FILE, 
	VM_SWAP,
	VM_STACK
};
struct vm_page {
	void *uaddr;
	enum vm_page_type type;
	struct file * file;
	off_t offset;
	bool writable;
	uint32_t read_bytes; 
	uint32_t zero_bytes;
	struct list_elem elem;
	int page_in_swap;
};

bool vm_install_page_fs(void *uaddr, struct file *f, off_t offset,
                        uint32_t read_bytes, uint32_t zero_bytes,
                        bool writable);
void vm_uninstall_page(struct thread *);
struct vm_page * get_vm_page(void *fault_addr, struct thread * t);
#endif