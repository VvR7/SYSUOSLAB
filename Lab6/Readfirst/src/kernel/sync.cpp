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
    initialize();
}

void Semaphore::initialize()
{
    this->rw=1;
    this->mutex = 1;
    semLock.initialize();
    waiting_mutex.initialize();
    waiting_rw.initialize();
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

// 简化接口实现
void Semaphore::P_mutex()
{
    P(mutex, waiting_mutex);
}

void Semaphore::V_mutex()
{
    V(mutex, waiting_mutex);
}

void Semaphore::P_rw()
{
    P(rw, waiting_rw);
}

void Semaphore::V_rw()
{
    V(rw, waiting_rw);
}