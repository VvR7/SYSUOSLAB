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
        //printf("pid: %d\n", programManager.running->pid);
    } while (key);
}

void SpinLock::unlock()
{
    bolt = 0;
}

Semaphore::Semaphore()
{
    initialize(0);
}

void Semaphore::initialize(uint32 n)
{
    this->empty = n;
    this->full = 0;
    this->mutex = 1;
    semLock.initialize();
    waiting_mutex.initialize();
    waiting_empty.initialize();
    waiting_full.initialize();
}

void Semaphore::P(uint32 &counter, List &waiting)
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
        waiting.push_back(&(cur->tagInGeneralList));
        cur->status = ProgramStatus::BLOCKED;

        semLock.unlock();
        programManager.schedule();
    }
}

void Semaphore::V(uint32 &counter, List &waiting)
{
    semLock.lock();
    ++counter;  
    if (waiting.size())
    {
        PCB *program = ListItem2PCB(waiting.front(), tagInGeneralList);
        waiting.pop_front();
        semLock.unlock();
        programManager.MESA_WakeUp(program);
    }
    else
    {
        semLock.unlock();
    }
}
void Semaphore::P_mutex()
{
    P(mutex, waiting_mutex);
}

void Semaphore::V_mutex()
{
    V(mutex, waiting_mutex);
}

void Semaphore::P_empty()
{
    P(empty, waiting_empty);
}

void Semaphore::V_empty()
{
    V(empty, waiting_empty);
}

void Semaphore::P_full()
{
    P(full, waiting_full);
}

void Semaphore::V_full()
{
    V(full, waiting_full);
}