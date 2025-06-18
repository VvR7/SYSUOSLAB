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
    uint32 mutex;  
    uint32 empty;  
    uint32 full;   
    List waiting_mutex;
    List waiting_empty;
    List waiting_full;
    SpinLock semLock;

public:
    Semaphore();
    void initialize(uint32 n);
    
    void P_mutex();   
    void V_mutex();   
    void P_empty();   
    void V_empty();   
    void P_full();    
    void V_full();    
    
    // 通用P和V操作
    void P(uint32 &counter, List &waiting);
    void V(uint32 &counter, List &waiting);
};
#endif