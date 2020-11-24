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
void *steal();

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {  //内存块
  struct run *next;
};

struct kmem{
  struct spinlock lock;
  struct run *freelist;
};

struct kmem kmems[NCPU]; //定义一个结构体数组

int getcpu(){  //获取当前cpu的id
  push_off();  //关中断
  int cpu = cpuid(); //获取当前cpu的id号
  pop_off();  //开中断
  return cpu;
}

void
kinit()
{
  //printf("[kinit] cpu id %d\n",getcpu());
  int i;
  for(i = 0;i < NCPU;i++){
    initlock(&kmems[i].lock, "kmem"); //初始化3个锁
  }
  
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end) //为所有运行freerange的cpu分配空闲的内存
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  int hart = getcpu();  //优先分配当前CPU的freelist中的内存块
  acquire(&kmems[hart].lock);  //上锁
  r->next = kmems[hart].freelist;  //当前cpu的freelist添加一项
  kmems[hart].freelist = r;
  release(&kmems[hart].lock);  //解锁
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  int hart = getcpu();  //优先分配当前CPU的freelist中的内存块
  acquire(&kmems[hart].lock);
  r = kmems[hart].freelist;
  if(r)  //如果当前cpu的freelist不为空，则将r添加到当前freelist
    kmems[hart].freelist = r->next;
  release(&kmems[hart].lock);

  if(!r){  //如果当前cpu的freelist为空，则向其他cpu窃取
    r = steal();
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}

void *steal(){   //窃取
  // printf("[steal] cpu id is %d\n",getcpu());
  struct run *rs = 0;
  int i;
  for(i = 0;i < NCPU;i++){
    if(holding(&kmems[i].lock)){
      continue; //检查每一个freelist，如果被占用则不能进行窃取，跳过
    }
    acquire(&kmems[i].lock);  //上锁
    if(kmems[i].freelist != 0){  //如果该cpu的freelist不为空
      rs = kmems[i].freelist;
      kmems[i].freelist = rs->next;  //头指针向下移动，让出一个内存块
      release(&kmems[i].lock);  //解锁
      return (void *)rs;
    }
    release(&kmems[i].lock);
  }
return (void *)rs;
}


// #include "types.h"
// #include "param.h"
// #include "memlayout.h"
// #include "spinlock.h"
// #include "riscv.h"
// #include "defs.h"

// void freerange(void *pa_start, void *pa_end);

// extern char end[]; // first address after kernel.
//                    // defined by kernel.ld.

// struct run {
//   struct run *next;
// };

// struct kmem{
//   struct spinlock lock;
//   struct run *freelist;
// };

// struct kmem kmems[3];

// int getcpu(){
//   push_off();
//   int cpu=cpuid();
//   pop_off();
//   return cpu;
// }


// void
// kinit()
// {

//   printf("[kinit] cpu id %d\n",getcpu());
//   for(int i=0;i<3;i++)
//     initlock(&kmems[i].lock, "kmem");
//   freerange(end, (void*)PHYSTOP);
// }

// void
// freerange(void *pa_start, void *pa_end)
// {
//   char *p;
//   p = (char*)PGROUNDUP((uint64)pa_start);
//   for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
//     kfree(p);
// }

// // Free the page of physical memory pointed at by v,
// // which normally should have been returned by a
// // call to kalloc().  (The exception is when
// // initializing the allocator; see kinit above.)
// void
// kfree(void *pa)
// {
//   struct run *r;

//   if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
//     panic("kfree");

//   // Fill with junk to catch dangling refs.
//   memset(pa, 1, PGSIZE);

//   r = (struct run*)pa;
//   int hart=getcpu();
//   acquire(&kmems[hart].lock);
//   r->next = kmems[hart].freelist;
//   kmems[hart].freelist = r;
//   release(&kmems[hart].lock);
// }

// void *
// steal(){
//   //("cpu id %d\n",getcpu());
//   struct run * rs=0;
//   for(int i=0;i<3;i++){
//     acquire(&kmems[i].lock);
//     if(kmems[i].freelist!=0){
//       rs=kmems[i].freelist;
//       kmems[i].freelist=rs->next;
//       release(&kmems[i].lock);
//       return (void *)rs;
//     }
//     release(&kmems[i].lock);
//   }
//   // 不管还有没有，都直接返回，不用panic
//   return (void *)rs;
// }
// // Allocate one 4096-byte page of physical memory.
// // Returns a pointer that the kernel can use.
// // Returns 0 if the memory cannot be allocated.
// void *
// kalloc(void)
// {
//   // printf("[kalloc] cpu id %d\n",getcpu());
//   // printkmem();
//   struct run *r;
//   int hart=getcpu();
//   acquire(&kmems[hart].lock);
//   r = kmems[hart].freelist;
//   if(r)
//     kmems[hart].freelist = r->next;
//   release(&kmems[hart].lock);
//   if(!r)
//   {
//     // 当前cpu对应的freelist为空 需要从其他cpu对应的freelist借
//     r=steal(hart);
//   }
//   if(r)
//     memset((char*)r, 5, PGSIZE); // fill with junk
//   return (void*)r;
// }