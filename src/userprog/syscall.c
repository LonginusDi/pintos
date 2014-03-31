#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"

#include "userprog/pagedir.h"

static void syscall_handler (struct intr_frame *);
static struct lock lock;
static int fd_num = 2;
static struct file * get_file_from_fd(int fd); 
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&lock);
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{ 
  int syscall = *(int*)(f->esp);
  switch(syscall) {

  	case SYS_WRITE: 
      {
        int fd = *(int*)(f->esp + 4);
        char * buf = *(char**)(f->esp + 8);
        int size = *(int*)(f->esp + 12);
        if (buf == NULL || size < 0)
          f->eax = -1;       
        else if(fd == 1) {  
          putbuf(buf, size);
          f->eax = size;
    	  }
        else if (fd == 0 || fd == 2) 
          f->eax = -1;
        else {
          struct file * file = get_file_from_fd(fd);
          if (file == NULL)
            f->eax = -1;
          else {
            lock_acquire(&lock);
            f->eax = file_write(file, buf, size);
            lock_release(&lock);
          }
        }
      }
    	break;
    case SYS_EXEC:
      f->eax = process_execute(*((char **)(f->esp + 4)));
      break;
    case SYS_EXIT: {
      //if (thread_current()->tid == 166)
        //printf("exit tid: %d\n", thread_current()->tid);
        struct thread * cur;
        cur = thread_current();
        if (!is_user_vaddr (f->esp + 4)) {
          printf("%s: exit(%d)\n", cur->name, -1);
          cur->status_code = -1;
          //printf("not failed\n");
        }
        else {
          printf("%s: exit(%d)\n", cur->name, *(int*)(f->esp + 4));
          cur->status_code = *(int*)(f->esp + 4);
          //printf("not failed\n");
        }
        thread_exit();
      }
      break;
    case SYS_CREATE: 
      {
        char * name = *(char**)(f->esp + 4);
        off_t size = *(off_t*)(f->esp + 8);
        if (name == NULL || size < 0) {
          printf("%s: exit(%d)\n", thread_current()->name, -1);
          thread_current()->status_code = -1;
          thread_exit();
        }
        lock_acquire(&lock);
        f->eax = filesys_create(name, size);
        lock_release(&lock);
      }
      break;
    case SYS_OPEN: {
        //printf("addr: %x\n", f->esp + 4);
        char * file_name = *((char **)(f->esp + 4));
        struct file * file;
        //printf("file_name:%s\n", file_name);
        lock_acquire(&lock);
        if (file_name && (file = filesys_open(file_name)) ) {
          file->fd = ++fd_num;
          list_push_back(&thread_current()->file_list, &file->elem);
          f->eax = file->fd;
        }
        else 
          f->eax = -1; 
        lock_release(&lock);
      }
      break;
    case SYS_READ: {
        int fd = *(int *)(f->esp + 4);
        char * buf = *(char **)(f->esp + 8);
        int size = *(off_t *)(f->esp + 12);
        if (!is_user_vaddr (buf)) {
          f->eax = -1;
          printf("%s: exit(%d)\n", thread_current()->name, -1);
          thread_current()->status_code = -1;
          thread_exit();
        }
        else if (buf == NULL || size < 0 || fd == 1 || fd == 2) {
          f->eax = -1;
        }
        else if (fd == 0)
          f->eax = input_getc();
        else {
          struct file * file = get_file_from_fd(fd);
          if (file) {
            lock_acquire(&lock);
            f->eax = file_read(file, buf, size);
            lock_release(&lock);
          }
          else 
            f->eax = -1;
        }
      }
      break;
    case SYS_FILESIZE: {
        int fd = *(int *)(f->esp + 4);
        f->eax = -1;
        if (fd > 2) {
          struct file * file = get_file_from_fd(fd);
          if (file != NULL) {
            lock_acquire(&lock);
            f->eax = file_length(file);
            lock_release(&lock);
          }
        }
      }
    break;
    case SYS_CLOSE: {
      int fd = *(int *)(f->esp + 4);
      struct file * file = get_file_from_fd(fd);
      f->eax = -1;
      if (file) {
        lock_acquire(&lock);
        list_remove(&file->elem);
        file_close(file);
        lock_release(&lock);
      }
    }
    break;
    case SYS_WAIT:
      f->eax = process_wait(*(tid_t *)(f->esp + 4));
      break;
    case SYS_HALT:
      shutdown_power_off();
      break;
    case SYS_SEEK: {
        int fd = *(int *)(f->esp + 4);
        off_t offset = *(off_t*)(f->esp + 8);
        struct file * file;
        if (fd < 3 || !(file = get_file_from_fd(fd)) )
          f->eax = -1;
        else {
          lock_acquire(&lock);
          file_seek(file, offset);
          lock_release(&lock);
        }
      }
      break;
    case SYS_REMOVE: {
        char * file_name = *(char**)(f->esp + 4);
        if (file_name == NULL)
        f->eax = -1;
        else {
          lock_acquire(&lock);
          filesys_remove(file_name);
          lock_release(&lock);
        }
      }
      break;
    case SYS_TELL: {
        int fd = *(int *)(f->esp + 4);
        struct file * file;
        if (fd < 3 || !(file = get_file_from_fd(fd)))
          f->eax = -1;
        else {
          lock_acquire(&lock);
          f->eax = file_tell(file);
          lock_release(&lock);
        }
      }
      break;
  }
}

static struct file * get_file_from_fd(int fd) {
  struct list_elem *p;
  struct thread * cur = thread_current();
  for (p = list_begin(&cur->file_list); p != list_end(&cur->file_list); p = list_next(p)) {
    struct file * t = list_entry(p, struct file, elem);
    if (t->fd == fd)
      return t;
  }
  return NULL;
}
