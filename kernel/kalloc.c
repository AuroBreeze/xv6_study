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
void __kfree(void *pa, int init);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
  char *ref_page;
  int page_cnt;
  char *end_;
} kmem;
int 
pagecnt(void *pa_start, void *pa_end)
{
  char *p;
  int cnt = 0;
  p = (char *)PGROUNDUP((uint64) pa_start);
  for(; p + PGSIZE < (char *)pa_end; p += PGSIZE)
    cnt++;
  return cnt;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  

  kmem.page_cnt = (PHYSTOP) / PGSIZE ;
  // printf("kmem page_cnt %d\n", kmem.page_cnt);
  // printf("PHYSTOP / PGSIZE = %d\n",(PHYSTOP - KERNBASE) / PGSIZE);
  kmem.ref_page = end;
  for(int i = 0; i < kmem.page_cnt; ++i){
    kmem.ref_page[i] = 0;
  }
  kmem.end_ = kmem.ref_page + kmem.page_cnt;

  freerange(kmem.end_, (void*)PHYSTOP);
 
}

int
page_index(void *pa)
{
  pa = (char *)PGROUNDDOWN((uint64)pa);
  int res =  ((uint64)pa) / PGSIZE;
  if(res < 0 || res > kmem.page_cnt){
    panic("page_index illegal");
  }
  return res;
}

void
incr(void *pa)
{
  int idx = page_index(pa);
  acquire(&kmem.lock);
  kmem.ref_page[idx]++;
  release(&kmem.lock);
}

void
desc(void *pa)
{
  int idx = page_index(pa);
  acquire(&kmem.lock);
  kmem.ref_page[idx]--;
  release(&kmem.lock);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    __kfree(p, 1);
}


// --- refcount support ---
// number of pages in physical memory that kalloc/kfree may touch
// index by pa/PGSIZE

static int *refcnt; // dynamically allocated array sized to (PHYSIZE/PGSIZE)
static struct spinlock ref_lock;
static int ref_npages = 0;

void
ref_init()
{
  initlock(&ref_lock, "refcnt");
  ref_npages = (PHYSTOP) / PGSIZE;

  refcnt = (int*)((uint64)end);
}

// for safety and simplicity across xv6 variants, we'll instead declare a static array;
static int refcnt_static[PHYSTOP / PGSIZE];

// we'll use refcnt_static as our refcnt storage;
#undef refcnt
#define refcnt refcnt_static

// end refcount setup

// Free the page of physical memory pointed at by pa,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
__kfree(void *pa, int init)
{
  int index = page_index(pa);
  if(kmem.ref_page[index]  > 1){
    desc(pa);
    return;
  }

  if(kmem.ref_page[index] == 1){
    desc(pa);
  }
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  uint64 idx = (uint64)pa / PGSIZE;

  if(!init){
    acquire(&ref_lock);
    if(refcnt[idx] <= 0)
      panic("kfree: refcnt <= 0");
    refcnt[idx]--;
    int cur = refcnt[idx];
    release(&ref_lock);

    if(cur > 0)
      return;
  }

  memset(pa, 1, PGSIZE);
  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

void
kfree(void *pa)
{
  __kfree(pa, 0);
}



// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r){
    uint64 pa = (uint64)r;
    uint64 idx = pa / PGSIZE;
    acquire(&ref_lock);
    refcnt[idx] = 1;
    release(&ref_lock);

    memset((char*)r, 5, PGSIZE); // fill with junk
    }
  return (void*)r;
}

void
krefinc(uint64 pa)
{
  // uint64 idx = pa  PGSIZE;
  acquire(&ref_lock);
  uint64 idx = pa / PGSIZE;
  if(refcnt[idx] <= 0)
    panic("krefinc: non-positive refcnt");
  refcnt[idx]++;
  release(&ref_lock);
}

int
krefdec(uint64 pa)
{
  uint64 idx = pa / PGSIZE;
  acquire(&ref_lock);
  if(refcnt[idx] <= 0){
    panic("krefdec: non-positive refcnt");
  }
  refcnt[idx]--;
  int v = refcnt[idx];
  release(&ref_lock);
  return v;
}

int
krefget(uint64 pa)
{
  uint64 idx = pa / PGSIZE;
  int v;
  acquire(&ref_lock);
  v = refcnt[idx];
  release(&ref_lock);
  return v;
}