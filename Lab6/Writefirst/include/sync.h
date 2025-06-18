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
    uint32 mutex;  // 互斥锁，初始为1,读者对count变量互斥访问
    uint32 rw;
    uint32 w;
    List waiting_mutex;
    List waiting_rw;
    List waiting_w;
    SpinLock semLock;

public:
    Semaphore();
    void initialize();
    
    void P_mutex();
    void V_mutex();
    void P_rw();
    void V_rw();
    void P_w();
    void V_w();
    
    // 通用P和V操作
    void P(uint32 &counter, List &waiting);
    void V(uint32 &counter, List &waiting);
};
#endif