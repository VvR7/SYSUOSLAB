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
int i=0;
static inline void flush_tlb_single(int addr) {
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}
void visit(char *p,int pagenum)  //访问第几页
{
    i++;
    printf("--------%d--------\n",i);
    p[pagenum*PAGE_SIZE]='a';
    flush_tlb_single((int)p+pagenum*PAGE_SIZE);
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
    char *p=(char *)memoryManager.allocatePages(AddressPoolType::KERNEL, 8);
    visit(p,1);
    visit(p,3);
    visit(p,4);
    visit(p,2);
    visit(p,5);
    visit(p,6);
    visit(p,3);
    visit(p,4);
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
