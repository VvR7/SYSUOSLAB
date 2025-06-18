#ifndef SYNC_H
#define SYNC_H

#include "os_type.h"
#include "list.h"
#include "thread.h"
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



// 哲学家状态枚举
enum State { THINKING, HUNGRY, EATING };

// 管程类
class Monitor
{
private:
    SpinLock lock; // 用于保证管程的互斥访问
    State state[5]; // 哲学家状态
    bool waiting[5]; // 替代List condition[5]，标记哲学家是否等待
    PCB* waitingPCB[5]; // 要存储等待的pcb
    
    // 测试是否可以进餐
    void test(int id);

public:
    Monitor();
    void initialize();
    void pickup(int id); // 拿起筷子
    void putdown(int id); // 放下筷子
};
#endif