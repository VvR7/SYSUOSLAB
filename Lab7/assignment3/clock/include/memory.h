#ifndef MEMORY_H
#define MEMORY_H

#include "address_pool.h"
#define MAX_QUEUE_SIZE 20000
enum AddressPoolType
{
    USER,
    KERNEL
};
struct item {
    item(int virtualAddress,int physicalAddress,bool vis=true)
    {
        this->virtualAddress=virtualAddress;
        this->physicalAddress=physicalAddress;
        this->vis=vis;
    }
    item(){}
    int virtualAddress;
    int physicalAddress;
    int valid;
    bool vis;
};
class Queue
{
    public:
      item queue[MAX_QUEUE_SIZE];
      int front;
      int rear;
      int cur;
      Queue()
      {
        front=0;
        rear=0;
        cur=0;
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
        while(size()>0&&queue[front].valid==0)
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
      int del(int index)
      {
        if (empty()) return -1;
        for (int i=index,next=(i+1)%MAX_QUEUE_SIZE;next!=rear;i=next,next=(i+1)%MAX_QUEUE_SIZE)
        {
            queue[i]=queue[next];
        }
        rear=(rear-1+MAX_QUEUE_SIZE)%MAX_QUEUE_SIZE;
        return 0;
      }
      int getcurpos()
      {
        return (cur-front+MAX_QUEUE_SIZE)%MAX_QUEUE_SIZE;
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
    Queue physicalPageQueue;
    int kernelPhysicalStartAddress;
    int userPhysicalStartAddress;
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

    int physicalPageID(int paddr);
    int virtualPageID(int vaddr);
    //访问引用位
    bool vis(int vaddr);
    //设置引用位
    void setVis(int vaddr);
    //寻找置换的页面
    int FindExchangePage();
};

#endif