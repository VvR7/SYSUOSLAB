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
    List waiting[5];
    SpinLock semLock;

public:
    uint32 chopsticks[5];
    Semaphore();
    void initialize();
    void P(int id);
    void V(int id);
};
#endif