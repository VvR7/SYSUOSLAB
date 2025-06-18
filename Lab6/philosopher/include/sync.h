#ifndef SYNC_H
#define SYNC_H

#include "os_type.h"
#include "list.h"

class SpinLock
{
private:
    uint32 bolt;

public:
    SpinLock();
    void initialize();
    void lock();
    void unlock();
};

class Semaphore
{
private:
    uint32 chopsticks[5];
    List waiting[5];
    List waiting_count;
    SpinLock semLock;
    uint32 counter;

public:
    Semaphore();
    void initialize(uint32 count = 4);  // 默认允许4个哲学家同时进入
    void P(int id);
    void V(int id);
    void P_count();
    void V_count();
};
#endif