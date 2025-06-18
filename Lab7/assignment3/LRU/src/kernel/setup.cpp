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
int count=0;
void visit(char *p,int pagenum)  //访问第几页
{
    count++;
    printf("--------%d--------\n",count);
    p[pagenum*PAGE_SIZE]='a';
    for (int i=memoryManager.physicalPageQueue.front;i!=memoryManager.physicalPageQueue.rear;i=(i+1)%MAX_QUEUE_SIZE)
    {
        memoryManager.physicalPageQueue.queue[i].lru++;
        item &u=memoryManager.physicalPageQueue.queue[i];
        if (u.virtualAddress==(int)p+pagenum*PAGE_SIZE)
        {
            u.lru=0;
        }
    }
}
void first_thread(void *arg)
{
    //第1个线程不可以返回
    stdio.moveCursor(0);
    for (int i = 0; i < 25 * 80; ++i)
    {
        stdio.print(' ');
    }
    stdio.moveCursor(0);
    char *p=(char *)memoryManager.allocatePages(AddressPoolType::KERNEL, 9);
    visit(p,1);
    visit(p,8);
    visit(p,1);
    visit(p,7);
    visit(p,8);
    visit(p,2);
    visit(p,7);
    visit(p,2);
    visit(p,1);
    visit(p,8);
    visit(p,3);
    visit(p,8);
    visit(p,2);
    visit(p,1);
    visit(p,3);
    visit(p,1);
    visit(p,7);
    visit(p,1);
    visit(p,3);
    visit(p,7);
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
    memoryManager.openPageMechanism();
    memoryManager.initialize();

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
