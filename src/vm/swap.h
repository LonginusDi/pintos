#ifndef VM_SWAP_H
#define VM_SWAP_H

void swap_init(void);
void swap_read(int, void *);
int swap_write(void *);
void swap_remove(int);


#endif