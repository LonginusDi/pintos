#ifndef VM_FRAME_H
#define VM_FRAME_H
#include "threads/thread.h"
#include "threads/palloc.h"
#include <list.h>
struct frame {
	void *paddr;
	struct list_elem elem;
	struct thread * thread;
	void *uaddr;
	bool done;
};

struct list frame_list;

void vm_frame_init(void);
void *vm_get_frame(enum palloc_flags flags, void * uaddr);
void vm_free_frame(void *);
void vm_frame_set_done(void *);
void vm_clear_reference(void);
#endif