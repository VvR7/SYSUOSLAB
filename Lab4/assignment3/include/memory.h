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
    MemoryManager();

    // 开启分页机制
    void openPageMechanism();

};

#endif