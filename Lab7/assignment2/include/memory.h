#ifndef MEMORY_H
#define MEMORY_H

#include "address_pool.h"

enum AddressPoolType
{
    USER,
    KERNEL
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
    
    enum AllocateType allocateType;
public:
    MemoryManager();

    // 初始化地址池
    void initialize(enum AllocateType allocateType);

    // 从type类型的物理地址池中分配count个连续的页
    // 成功，返回起始地址；失败，返回0
    int allocatePhysicalPages(enum AddressPoolType type, const int count);

    // 释放从paddr开始的count个物理页
    void releasePhysicalPages(enum AddressPoolType type, const int startAddress, const int count);

    // 释放从index开始的count个物理页
    void releasepage(enum AddressPoolType type, const int index, const int count);

    // 获取内存总容量
    int getTotalMemory();

};

#endif