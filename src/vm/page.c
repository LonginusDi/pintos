#include "vm/page.h"
bool vm_install_page_fs(void *uaddr, struct file *f, off_t offset,
                        uint32_t read_bytes, uint32_t zero_bytes,
                        bool writable) {
	struct thread * cur = thread_current();
	struct vm_page *page;
  page = malloc(sizeof(struct vm_page));
  if(!page) 
  	return false;

  page->uaddr = uaddr;
  page->type = VM_FILE;
  page->file = f;
  page->offset = offset;
  page->read_bytes = read_bytes;
  page->zero_bytes = zero_bytes;
  page->writable = writable;

  list_push_back(&cur->supp_page_dir, &page->elem);
  return true;
}

bool vm_install_page_stack(void *uaddr) {
  struct thread * cur = thread_current();
  struct vm_page *page;
  page = malloc(sizeof(struct vm_page));
  if(!page) 
    return false;

  page->uaddr = uaddr;
  page->type = VM_STACK;
  page->writable = true;

  list_push_back(&cur->supp_page_dir, &page->elem);
  return true;
}

void vm_uninstall_page(void *uaddr) {
  struct list_elem *p;
  struct thread *cur = thread_current();
  for (p = list_begin(&cur->supp_page_dir); p != list_end(&cur->supp_page_dir); p = list_next(p)) {
    struct vm_page *t = list_entry(p, struct vm_page, elem);
    if (t->uaddr == uaddr) {
      list_remove(p);
      free(t);
      break;
    }
  }
}

struct vm_page * get_vm_page(void *fault_addr, struct thread * t){
  struct list_elem *p;
  for (p = list_begin(&t->supp_page_dir); p != list_end(&t->supp_page_dir); p = list_next(p)) {
    struct vm_page *v = list_entry(p, struct vm_page, elem);
    if (v->uaddr == fault_addr) {
      return v;
    }
  }
  return NULL;
}