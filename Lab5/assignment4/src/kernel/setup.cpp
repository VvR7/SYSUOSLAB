#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"

// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;

// 全局变量，用于控制线程的创建
int system_ticks = -1;
bool second_created = false;
bool third_created = false;

void third_thread(void *arg) {
    printf("Processing pid %d name \"%s\": Thread 3\n", programManager.running->pid, programManager.running->name);
    asm_halt();
}

void second_thread(void *arg) {
    printf("Processing pid %d name \"%s\": Thread 2\n", programManager.running->pid, programManager.running->name);
    asm_halt();
}
void first_thread(void *arg)
{
    printf("Processing pid %d name \"%s\": Thread 1\n", programManager.running->pid, programManager.running->name);
    asm_halt();
}
void init_thread(void *arg)
{
    printf("Processing pid %d name \"%s\": init thread\n", programManager.running->pid, programManager.running->name);
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

    int pid = programManager.executeThread(init_thread, nullptr, "init thread", 100000000);
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
