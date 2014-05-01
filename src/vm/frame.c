#include "vm/frame.h"
#include "threads/thread.h"
#include "userprog/pagedir.h"
#include "vm/swap.h"
#include "vm/page.h"
static struct lock lock;
static void* vm_evict_frame(void *); 
static struct frame* pick_evicted_frame();
static struct frame* get_frame_by_addr(void *frame);
static int temp = 0;
void vm_frame_init() {
	list_init(&frame_list);
	lock_init(&lock);
}

void * vm_get_frame(enum palloc_flags flags, void *uaddr) {
	if (flags & PAL_USER == 0)
		return NULL;
	void *frame;
	frame = palloc_get_page(flags);
	if (frame) {
		struct frame * f;
		f = malloc(sizeof (struct frame));
		f->paddr = frame;
		f->uaddr = uaddr;
		f->thread = thread_current();
		f->done = false;
		lock_acquire(&lock);
		list_push_back(&frame_list, &f->elem);
		lock_release(&lock);
	}
	else {
		frame = vm_evict_frame(uaddr);
	}
	return frame;
}

void vm_free_frame(struct thread * thr) {
	lock_acquire(&lock);
	struct list_elem *p = list_begin(&frame_list);
	while (p != list_end(&frame_list)) {
		struct frame * t = list_entry(p, struct frame, elem);
		p = list_next(p);
		if (t->thread && t->thread->tid == thr->tid) {
			t->thread = NULL;
		}
	}
	vm_uninstall_page(thr);
	lock_release(&lock);
}

void vm_frame_set_done(void * frame) {
	struct frame * f = get_frame_by_addr(frame);
	f->done = true;
}

void vm_clear_reference() {
	if (!lock_try_acquire(&lock))
		return;
	if (list_size(&frame_list) == 0) {
		lock_release(&lock);
		return;
	}
	struct list_elem *p;
	for (p = list_begin(&frame_list); p != list_end(&frame_list); p = list_next(p)) {
		struct frame * t = list_entry(p, struct frame, elem);
		struct thread * thr = t->thread;
		if (thr != NULL)
			pagedir_set_accessed(thr->pagedir, t->uaddr, false);
	}
	lock_release(&lock);
}

static void * vm_evict_frame(void* nuaddr) {
	volatile struct frame * evicted;
	volatile struct thread * thread;
	volatile void * uaddr, * paddr;
	lock_acquire(&lock);
	//printf("temp: %d\n", temp);
	evicted = pick_evicted_frame();
	thread = evicted->thread;
	uaddr = evicted->uaddr;
	paddr = evicted->paddr;
	evicted->done = false;
	if (thread != NULL) {
		bool dirty = pagedir_is_dirty(thread->pagedir, uaddr);
		//printf("check: %d\n", temp);
		pagedir_clear_page(thread->pagedir, uaddr);
		struct vm_page * page = get_vm_page(uaddr, thread);
		if (page->type == VM_FILE && dirty) {
			page->type = VM_SWAP;
			page->page_in_swap = swap_write(paddr);
		}
		else if (page->type == VM_SWAP) {
			//printf("swap\n");
			page->page_in_swap = swap_write(paddr);
		}
		else if (page->type == VM_STACK && dirty) {
			//printf("stack\n");
			page->type = VM_SWAP;
			page->page_in_swap = swap_write(paddr);
		}		
	}	
	evicted->uaddr = nuaddr;
	evicted->thread = thread_current();
	evicted->done = false;
	list_push_back(&frame_list, &evicted->elem);	
	if (evicted->paddr == NULL)
		printf("NULL");
	lock_release(&lock);
	return evicted->paddr;
}

static struct frame* pick_evicted_frame() {
	
	struct list_elem * elem;
	struct frame * t = NULL;
	for (elem = list_begin(&frame_list); elem != list_end(&frame_list); elem = list_next(elem)) {
		struct frame * f = list_entry(elem, struct frame, elem);
		struct thread * thr = f->thread;
		if (thr == NULL) {
			t = f;
			break;
		}
		else {
			if (f->done && !pagedir_is_accessed(thr->pagedir, f->uaddr)) {
				t = f;
				break;
			}
		}
	}
	if (t == NULL) {
		for (elem = list_begin(&frame_list); elem != list_end(&frame_list); elem = list_next(elem)) {
			struct frame * f = list_entry(elem, struct frame, elem);
			struct thread * thr = f->thread;
			if (thr == NULL) {
				t = f;
				break;
			}
			else {
				if (f->done) {
					t = f;
					break;
				}
			}
		}
	}
	list_remove(&t->elem);	
	return t;
}

static struct frame* get_frame_by_addr(void *frame) {
	lock_acquire(&lock);
	struct list_elem * elem;
	struct frame * t;
	for (elem = list_begin(&frame_list); elem != list_end(&frame_list); elem = list_next(elem)) {
		struct frame * f = list_entry(elem, struct frame, elem);
		if (f->paddr == frame) {
			t = f;
			break;
		}
	}
	lock_release(&lock);	
	return t;
}