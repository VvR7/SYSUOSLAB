#include "sync.h"
#include "asm_utils.h"
#include "stdio.h"
#include "os_modules.h"
#include "program.h"

SpinLock::SpinLock()
{
    initialize();
}

void SpinLock::initialize()
{
    bolt = 0;
}

void SpinLock::lock()
{
    uint32 key = 1;

    do
    {
        asm_atomic_exchange(&key, &bolt);
    } while (key);
}

void SpinLock::unlock()
{
    bolt = 0;
}






// Monitor实现
Monitor::Monitor()
{
    initialize();
}

void Monitor::initialize()
{
    lock.initialize();
    
    // 初始化所有哲学家状态为思考
    for (int i = 0; i < 5; i++)
    {
        state[i] = THINKING;
        waiting[i] = false;
        waitingPCB[i] = nullptr;
    }
}

// 测试哲学家是否可以进餐
void Monitor::test(int id) {
    int left = (id + 4) % 5;
    int right = (id + 1) % 5;
    
    if (state[id] == HUNGRY && state[left] != EATING && state[right] != EATING) {
        state[id] = EATING;
        
        if (waiting[id]) {
            waiting[id] = false;
            programManager.MESA_WakeUp(waitingPCB[id]);
            waitingPCB[id] = nullptr;
        }
    }
}

// 拿起筷子
void Monitor::pickup(int id) {
    lock.lock();
    
    state[id] = HUNGRY;
    test(id);
    
    if (state[id] != EATING) {
        waiting[id] = true;
        waitingPCB[id] = programManager.running;
        programManager.running->status = ProgramStatus::BLOCKED;
        
        lock.unlock();
        programManager.schedule();
    } else {
        lock.unlock();
    }
}

// 放下筷子
void Monitor::putdown(int id)
{
    lock.lock();
    
    // 设置状态为思考
    state[id] = THINKING;
    
    // 测试左右邻居是否可以进餐
    int left = (id + 4) % 5;
    int right = (id + 1) % 5;
    
    test(left);
    test(right);
    
    lock.unlock();
}
