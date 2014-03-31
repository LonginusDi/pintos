#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"
void syscall_init (void);
struct lock lock;
#endif /* userprog/syscall.h */
