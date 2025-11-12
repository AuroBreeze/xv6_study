
// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel, defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];

static struct run* steal(int cid);

// init per-CPU locks and take all free pages on the booting CPU
void
kinit(void)
{
  for (int i = 0; i < NCPU; i++) {
    // 重要：传递静态存储期的名字，避免悬空指针
    initlock(&kmem[i].lock, "kmem");
    kmem[i].freelist = 0;
  }
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p = (char*)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free one 4096-byte page.
void
kfree(void *pa)
{
  if (((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  struct run *r = (struct run*)pa;

  // 为防止在读取 cid 与后续使用之间被迁移，短暂关中断获取 cid
  push_off();
  int cid = cpuid();   // 原版 cpuid，不改动
  pop_off();

  acquire(&kmem[cid].lock);
  r->next = kmem[cid].freelist;
  kmem[cid].freelist = r;
  release(&kmem[cid].lock);
}

// Try to steal one page from another CPU's freelist.

#define STEAL_BATCH 16

static struct run *steal(int cid) {
  int start = (cid + 1) % NCPU;
  for (int i = 0; i < NCPU; i++) {
    int target = (start + i) % NCPU;
    if (target == cid) continue;

    acquire(&kmem[target].lock);

    struct run *r = kmem[target].freelist;
    if(!r){
      release(&kmem[target].lock);
      continue;
    }
    int cnt = 15;
    struct run *tail = r;
    while(tail && tail->next && cnt-->0){
      tail = tail->next;
    }
    struct run *remain = tail->next;
    tail->next = 0; // 切断链表

    kmem[target].freelist = remain;
    release(&kmem[target].lock);

    acquire(&kmem[cid].lock);
    tail->next = kmem[cid].freelist;
    kmem[cid].freelist = r;

    struct run *page = kmem[cid].freelist;
    if(page){
      kmem[cid].freelist = page->next;
    }
    release(&kmem[cid].lock);

    return page;
  }
  return 0;
}


// Allocate one 4096-byte page.
void *
kalloc(void)
{
  struct run *r;

  // 同理，读取 cid 前后最小作用域关中断
  push_off();
  int cid = cpuid();
  pop_off();

  acquire(&kmem[cid].lock);
  r = kmem[cid].freelist;
  if (r) {
    kmem[cid].freelist = r->next;
    release(&kmem[cid].lock);
  } else {
    release(&kmem[cid].lock);
    r = steal(cid); // r = kmem[target].freelist
    if (!r) {
      return 0;
    }
  }

  memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

