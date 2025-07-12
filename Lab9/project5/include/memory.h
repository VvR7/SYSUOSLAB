#ifndef MEMORY_H
#define MEMORY_H

#include "address_pool.h"
#include "disk.h"
#include "sync.h"
#define MAX_QUEUE_SIZE 15985
enum AddressPoolType
{
    USER,
    KERNEL
};
struct item {
    item(int virtualAddress,int physicalAddress,int count=1,int pid=0)
    {
        this->virtualAddress=virtualAddress;
        this->physicalAddress=physicalAddress;
    }
    item(){}
    int virtualAddress;
    int physicalAddress;
};
class Queue
{
    public:
      item queue[MAX_QUEUE_SIZE];
      int front;
      int rear;
      Queue()
      {
        front=0;
        rear=0;
      }
      void push(item value)
      {
        queue[rear]=value;
        rear=(rear+1)%MAX_QUEUE_SIZE;
      }
      void pop()
      {
        front=(front+1)%MAX_QUEUE_SIZE;
      }
      item getFront()
      {
        while(size()>0)
        {
            front=(front+1)%MAX_QUEUE_SIZE;
        }
        return queue[front];
      }
      bool empty()
      {
        return front==rear;
      }
      int size()
      {
        return (rear-front+MAX_QUEUE_SIZE)%MAX_QUEUE_SIZE;
      }
};
struct free_block {
    int vaddr;
    int size;
    free_block(int vaddr,int size)
    {
        this->vaddr=vaddr;
        this->size=size;
    }
    free_block(){}
};
class Free_queue
{
    public:
      free_block queue[MAX_QUEUE_SIZE];
      int front;
      int rear;
      Free_queue()
      {
        front=0;
        rear=0;
      }
      void push(free_block value) 
      {
        queue[rear]=value;
        rear=(rear+1)%MAX_QUEUE_SIZE;
      }
      void pop()
      {
        front=(front+1)%MAX_QUEUE_SIZE;
      }
      free_block getFront()
      {
        return queue[front];
      }
      int size()
      {
        return (rear-front+MAX_QUEUE_SIZE)%MAX_QUEUE_SIZE;
      }
      void del(int index)
      {
        for (int i=index;i!=rear;i=(i+1)%MAX_QUEUE_SIZE)
        {
            queue[i]=queue[(i+1)%MAX_QUEUE_SIZE];
        }
        rear=(rear-1+MAX_QUEUE_SIZE)%MAX_QUEUE_SIZE;
      }
};
struct read_only_page {
    int paddr;
    int count;
    read_only_page(int paddr,int count):paddr(paddr),count(count){}
    read_only_page(){}
};
struct Read_only_page_queue {
    read_only_page queue[MAX_QUEUE_SIZE];
    int front;
    int rear;
    Read_only_page_queue()
    {
        front=0;
        rear=0;
    }
    int find_page(int paddr) {
        for (int i=front;i!=rear;i=(i+1)%MAX_QUEUE_SIZE)
        {
            if (queue[i].paddr==paddr) {
                return i;
            }
        }
        return -1;
    }
    void add_page(int paddr) {
        int index=find_page(paddr);
        if (index==-1) {
            queue[rear]=read_only_page(paddr,1);
            rear=(rear+1)%MAX_QUEUE_SIZE;
        } else {
            queue[index].count++;
        }
    }
    void pop_page(int paddr) {
        int index=find_page(paddr);
        if (index!=-1) {
            queue[index].count--;
        }
    }
};
class MemoryManager
{
public:
    // 可管理的内存容量
    int totalMemory;
    // 内核物理地址池
    AddressPool kernelPhysical;
    // 用户物理地址池
    AddressPool userPhysical;
    // 内核虚拟地址池
    AddressPool kernelVirtual;
    //Queue KernelphysicalPageQueue;  //内核物理页队列
    Queue UserphysicalPageQueue;    //用户物理页队列
    int kernelPhysicalStartAddress;
    int userPhysicalStartAddress;
    Free_queue free_queue;
    SwapAreaManager swapAreaManager;
    Read_only_page_queue read_only_page_queue;

    SpinLock allocate_lock;
    SpinLock malloc_lock;
    SpinLock free_lock;
    SpinLock swap_lock;
public:
    MemoryManager();

    // 初始化地址池
    void initialize();

    // 从type类型的物理地址池中分配count个连续的页
    // 成功，返回起始地址；失败，返回0
    int allocatePhysicalPages(enum AddressPoolType type, const int count);

    // 释放从paddr开始的count个物理页
    void releasePhysicalPages(enum AddressPoolType type, const int startAddress, const int count);

    // 获取内存总容量
    int getTotalMemory();

    // 开启分页机制
    void openPageMechanism();

    // 页内存分配
    int allocatePages(enum AddressPoolType type, const int count);

    // 虚拟页分配
    int allocateVirtualPages(enum AddressPoolType type, const int count);

    // 建立虚拟页到物理页的联系
    bool connectPhysicalVirtualPage(const int virtualAddress, const int physicalPageAddress);

    // 计算virtualAddress的页目录项的虚拟地址
    int toPDE(const int virtualAddress);

    // 计算virtualAddress的页表项的虚拟地址
    int toPTE(const int virtualAddress);

    // 页内存释放
    void releasePages(enum AddressPoolType type, const int virtualAddress, const int count);    

    // 找到虚拟地址对应的物理地址
    int vaddr2paddr(int vaddr);

    // 释放虚拟页
    void releaseVirtualPages(enum AddressPoolType type, const int vaddr, const int count);

    uint32 exchangepage(int vaddr);
    int physicalPageID(int paddr);
    int virtualPageID(int vaddr);

    // 页的换出
    void swapOut(uint32 vaddr); //虚拟地址对应页表的有效位为0，31-12位设为pagenum，标记在外存的位置

    //malloc
    void *malloc(int size);  //申请size字节的内存

    //free
    void free(void *ptr,int size);  //释放ptr指向的内存

    void COW_page_default_management(int vaddr);


};


#endif