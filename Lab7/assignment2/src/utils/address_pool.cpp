#include "address_pool.h"
#include "os_constant.h"

AddressPool::AddressPool()
{
}

// 设置地址池BitMap
void AddressPool::initialize(char *bitmap, const int length, const int startAddress, enum AllocateType allocateType)
{
    resources.initialize(bitmap, length, allocateType);
    this->startAddress = startAddress;
}

// 从地址池中分配count个连续页
int AddressPool::allocate(const int count)
{
    int start = resources.allocate(count);
    return (start == -1) ? -1 : (start * PAGE_SIZE + startAddress);
}

// 释放若干页的空间
void AddressPool::release(const int address, const int amount)
{
    resources.release((address - startAddress) / PAGE_SIZE, amount);
}
void AddressPool::releasepage(const int index,const int count)
{
    resources.release(index,count);
}
