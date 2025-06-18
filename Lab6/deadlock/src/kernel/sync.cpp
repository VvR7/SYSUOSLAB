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
    initialize();
}

void Semaphore::initialize()
{
    for (int i = 0; i < 5; i++)
    {
        chopsticks[i] = 1;
        waiting[i].initialize();
    }
    semLock.initialize();
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