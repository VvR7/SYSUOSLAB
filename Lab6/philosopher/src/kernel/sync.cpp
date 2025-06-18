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

Semaphore::Semaphore()
{
    initialize(4);  // 默认允许4个哲学家同时进入
}

void Semaphore::initialize(uint32 count)
{
    for (int i = 0; i < 5; i++)
    {
        chopsticks[i] = 1;
        waiting[i].initialize();
    }
    semLock.initialize();
    counter = count;
    waiting_count.initialize();
}

void Semaphore::P(int id)
{
    PCB *cur = nullptr;

    while (true)
    {
        semLock.lock();
        if (chopsticks[id] > 0)
        {
            --chopsticks[id];
            semLock.unlock();
            return;
        }

        cur = programManager.running;
        waiting[id].push_back(&(cur->tagInGeneralList));
        cur->status = ProgramStatus::BLOCKED;

        semLock.unlock();
        programManager.schedule();
    }
}

void Semaphore::P_count()
{
    PCB *cur = nullptr;

    while (true)
    {
        semLock.lock();
        if (counter > 0)
        {
            --counter;
            semLock.unlock();
            return;
        }

        cur = programManager.running;
        waiting_count.push_back(&(cur->tagInGeneralList));
        cur->status = ProgramStatus::BLOCKED;

        semLock.unlock();
        programManager.schedule();
    }
}

void Semaphore::V(int id)
{
    semLock.lock();
    ++chopsticks[id];
    if (waiting[id].size())
    {
        PCB *program = ListItem2PCB(waiting[id].front(), tagInGeneralList);
        waiting[id].pop_front();
        semLock.unlock();
        programManager.MESA_WakeUp(program);
    }
    else
    {
        semLock.unlock();
    }
}

void Semaphore::V_count()
{
    semLock.lock();
    ++counter;
    if (waiting_count.size())
    {
        PCB *program = ListItem2PCB(waiting_count.front(), tagInGeneralList);
        waiting_count.pop_front();
        semLock.unlock();
        programManager.MESA_WakeUp(program);
    }
    else
    {
        semLock.unlock();
    }
}
