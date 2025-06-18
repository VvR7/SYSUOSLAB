#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"
#include "memory.h"

// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;
// 内存管理器
MemoryManager memoryManager;
/*
total pages:15984

*/
void first_thread(void *arg)
{
    int start = memoryManager.allocatePhysicalPages(AddressPoolType::KERNEL, 11000);
    memoryManager.releasepage(AddressPoolType::KERNEL, 2000, 8000);
    int start2 = memoryManager.allocatePhysicalPages(AddressPoolType::KERNEL, 3500);
    int start3 = memoryManager.allocatePhysicalPages(AddressPoolType::KERNEL, 2500);
    asm_halt();
}

extern "C" void setup_kernel()
{

    // 中断管理器
    interruptManager.initialize();
    interruptManager.enableTimeInterrupt();
    interruptManager.setTimeInterrupt((void *)asm_time_interrupt_handler);

    // 输出管理器
    stdio.initialize();

    // 进程/线程管理器
    programManager.initialize();

    // 内存管理器
    //memoryManager.initialize(AllocateType::BEST_FIT);
    //memoryManager.initialize(AllocateType::FIRST_FIT);
    memoryManager.initialize(AllocateType::WORST_FIT);
    // 创建第一个线程
    int pid = programManager.executeThread(first_thread, nullptr, "first thread", 1);
    if (pid == -1)
    {
        printf("can not execute thread\n");
        asm_halt();
    }

    ListItem *item = programManager.readyPrograms.front();
    PCB *firstThread = ListItem2PCB(item, tagInGeneralList);
    firstThread->status = RUNNING;
    programManager.readyPrograms.pop_front();
    programManager.running = firstThread;
    asm_switch_thread(0, firstThread);

    asm_halt();
}
