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
PCB* Init_PCB = nullptr;
void print_children_pid(PCB *parent) {
    printf("%d 's children pid list:\n", parent->pid);
    ListItem *current = parent->children.front();
    while (current) {
        PCB *child = ListItem2PCB(current, tagInChildrenList);
        if (child->status != DEAD) {
            printf("%d\n", child->pid);
        }
        current = current->next;
    }
}
void four_thread(void *arg) {
    printf("pid %d name \"%s\": The fourth thread\n", programManager.running->pid, programManager.running->name);
    print_children_pid(Init_PCB);
    asm_halt();
}
void third_thread(void *arg) {
    printf("pid %d name \"%s\": The third thread\n", programManager.running->pid, programManager.running->name);
    programManager.executeThread(four_thread, nullptr, "fourth thread", 1, programManager.running);
    print_children_pid(programManager.running);
    asm_halt();
}
void second_thread(void *arg) {
    printf("pid %d name \"%s\": The second thread\n", programManager.running->pid, programManager.running->name);
    //asm_halt();
}
void first_thread(void *arg)
{
    // 第1个线程不可以返回
    printf("pid %d name \"%s\": Hello World!\n", programManager.running->pid, programManager.running->name);
    if (!programManager.running->pid)
    {
        programManager.executeThread(second_thread, nullptr, "second thread", 1, programManager.running);
        programManager.executeThread(third_thread, nullptr, "third thread", 1, programManager.running);
    }
    print_children_pid(programManager.running);
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

    // 创建第一个线程
    int pid = programManager.executeThread(first_thread, nullptr, "first thread", 1,nullptr);
    if (pid == -1)
    {
        printf("can not execute thread\n");
        asm_halt();
    }

    ListItem *item = programManager.readyPrograms.front();
    PCB *firstThread = ListItem2PCB(item, tagInGeneralList);
    Init_PCB = firstThread;
    firstThread->status = RUNNING;
    programManager.readyPrograms.pop_front();
    programManager.running = firstThread;
    asm_switch_thread(0, firstThread);

    asm_halt();
}
