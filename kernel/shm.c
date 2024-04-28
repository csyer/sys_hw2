#include "param.h"
#include "types.h"
#include "memlayout.h"
#include "elf.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "spinlock.h"
#include "proc.h"
#include "shm.h"

struct shm_segment shm_table[MAX_SHM_SEGMENTS];

void shm_init(void) {
    int i;
    for(i = 0; i < MAX_SHM_SEGMENTS; i++) {
        shm_table[i].id = 0;
        shm_table[i].refcount = 0;
        shm_table[i].addr = 0;
        shm_table[i].used = 0;
    }
}

uint64 shm_create(int size, int permission) {
    int i;
    for(i = 0; i < MAX_SHM_SEGMENTS; i++) {
        if(shm_table[i].used == 0) {
            shm_table[i].id = i;
            shm_table[i].refcount = 1;
            shm_table[i].addr = (uint64)kalloc(); // 分配一页内存
            memset((char*)shm_table[i].addr, 0, PGSIZE);
            shm_table[i].used = 1;
            return i;
        }
    }
    return -1; // 没有空闲段可用
}

uint64 shm_attach(int shm_id) {
    struct proc *curproc = myproc();
    if(shm_id < 0 || shm_id >= MAX_SHM_SEGMENTS || shm_table[shm_id].used == 0)
        return -1;
    
    if (mappages(curproc->pagetable, curproc->sz, PGSIZE, shm_table[shm_id].addr, PTE_W|PTE_U) < 0)
        return -1;
    curproc->sz += PGSIZE;
    shm_table[shm_id].refcount++;
    return 0;
}

uint64 shm_detach(int shm_id) {
    struct proc *curproc = myproc();
    struct shm_segment *seg;

    if (shm_id < 0 || shm_id >= MAX_SHM_SEGMENTS)
        return -1;  // 验证 shm_id 是否有效

    seg = &shm_table[shm_id];

    if (seg->used == 0 || seg->refcount == 0)
        return -1;  // 检查内存段是否已经被使用

    uvmunmap(curproc->pagetable, (uint64)(curproc->sz - PGSIZE), PGSIZE, 1);
    curproc->sz -= PGSIZE;

    seg->refcount--;
    if (seg->refcount == 0) {
        kfree((void*)seg->addr);
        seg->addr = 0;
        seg->used = 0;  // 标记段为未使用
    }

    return 0;
}

uint64 shm_ctl(int shm_id, int command, int permission) {
    struct shm_segment *seg;

    if (shm_id < 0 || shm_id >= MAX_SHM_SEGMENTS)
        return -1;  // 验证 shm_id 是否有效

    seg = &shm_table[shm_id];
    if (seg->used == 0)
        return -1;  // 检查段是否在使用

    switch (command) {
        case SHM_GET_PERM:
            return permission;

        case SHM_SET_PERM:
            if (permission & ~(PTE_W | PTE_U))
                return -1; // 验证新权限
            seg->permission = permission;
            return 0;

        default:
            return -1;  // 不支持的命令
    }
}
