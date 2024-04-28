#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_getclk(void)
{
  return *(uint64*) CLINT_MTIME;
}

// 创建共享内存页
uint64
sys_shm_create(void)
{
  int size;
  if(argint(0, &size) < 0)
    return -1;
  return shm_create(size);
}

// 绑定共享内存页
void*
sys_shm_attach(void)
{
  int shm_id;
  if(argint(0, &shm_id) < 0)
    return -1;
  return shm_attach(shm_id);
}

// 释放共享内存页
uint64
sys_shm_detach(void)
{
  int shm_id;
  if(argint(0, &shm_id) < 0)
    return -1;
  return shm_detach(shm_id);
}

// 查询和修改共享内存页配置
uint64
sys_shm_ctl(void)
{
  int shm_id, command;
  void* buf;
  
  if(argint(0, &shm_id) < 0 || argint(1, &command) < 0 || argptr(2, (void*)&buf, sizeof(void*)) < 0)
    return -1;
  return shm_ctl(shm_id, command, buf);
}