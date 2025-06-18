#include "program.h"
#include "stdlib.h"
#include "interrupt.h"
#include "asm_utils.h"
#include "stdio.h"
#include "thread.h"
#include "os_modules.h"

const int PCB_SIZE = 4096;                   // PCB的大小，4KB。
char PCB_SET[PCB_SIZE * MAX_PROGRAM_AMOUNT]; // 存放PCB的数组，预留了MAX_PROGRAM_AMOUNT个PCB的大小空间。
bool PCB_SET_STATUS[MAX_PROGRAM_AMOUNT];     // PCB的分配状态，true表示已经分配，false表示未分配。
extern int system_ticks;
ProgramManager::ProgramManager()
{
    initialize();
}

void ProgramManager::initialize()
{
    allPrograms.initialize();
    readyPrograms.initialize();
    running = nullptr;

    for (int i = 0; i < MAX_PROGRAM_AMOUNT; ++i)
    {
        PCB_SET_STATUS[i] = false;
    }
}

int ProgramManager::executeThread(ThreadFunction function, void *parameter, const char *name, int priority)
{
    // 关中断，防止创建线程的过程被打断
    bool status = interruptManager.getInterruptStatus();
    interruptManager.disableInterrupt();

    // 分配一页作为PCB
    PCB *thread = allocatePCB();

    if (!thread)
        return -1;

    // 初始化分配的页
    memset(thread, 0, PCB_SIZE);

    for (int i = 0; i < MAX_PROGRAM_NAME && name[i]; ++i)
    {
        thread->name[i] = name[i];
    }

    thread->status = ProgramStatus::READY;
    thread->priority = priority;  // priority现在表示总的执行时间
    thread->ticks = priority;     // ticks初始值等于总执行时间
    thread->ticksPassedBy = 0;    // 已执行时间初始化为0
    thread->pid = ((int)thread - (int)PCB_SET) / PCB_SIZE;

    // 线程栈
    thread->stack = (int *)((int)thread + PCB_SIZE);
    thread->stack -= 7;
    thread->stack[0] = 0;  //ebp
    thread->stack[1] = 0;  //ebx
    thread->stack[2] = 0;  //edi
    thread->stack[3] = 0;  //esi
    thread->stack[4] = (int)function;
    thread->stack[5] = (int)program_exit;
    thread->stack[6] = (int)parameter;

    allPrograms.push_back(&(thread->tagInAllList));
    readyPrograms.push_back(&(thread->tagInGeneralList));

    // 恢复中断
    interruptManager.setInterruptStatus(status);

    return thread->pid;
}

void ProgramManager::schedule()
{
    bool status = interruptManager.getInterruptStatus();
    interruptManager.disableInterrupt();

    if (readyPrograms.size() == 0)
    {
        interruptManager.setInterruptStatus(status);
        return;
    }

    // 更新最短剩余时间
    update_shortest_remain_time();
    if (running->status == ProgramStatus::RUNNING)
    {
        // 检查是否需要抢占
        if (shortest_program && running->ticks > shortest_remain_time) {
            // 当前运行进程的剩余时间比就绪队列中最短的还要长，需要抢占
            running->status = ProgramStatus::READY;
            readyPrograms.push_back(&(running->tagInGeneralList));
        } 
        else if (running->ticksPassedBy >= running->priority) {
            // 进程已经完成了预定的执行时间
            running->status = ProgramStatus::DEAD;
            printf("system tick:%d dead: %d\n", system_ticks, running->pid);
            releasePCB(running);
        }
        else {
            // 不需要抢占且未完成执行，继续运行当前进程
            interruptManager.setInterruptStatus(status);
            return;
        }
    }
    else if (running->status == ProgramStatus::DEAD)
    {
        releasePCB(running);
    }

    // 选择最短剩余时间的进程
    ListItem *item;
    if (shortest_program) {
        item = &(shortest_program->tagInGeneralList);
        readyPrograms.erase(item);
        readyPrograms.push_front(item);
    }
    
    item = readyPrograms.front();
    PCB *next = ListItem2PCB(item, tagInGeneralList);
    PCB *cur = running;
    printf("System_ticks:%d change: %d to %d\n",system_ticks,cur->pid, next->pid);
    next->status = ProgramStatus::RUNNING;
    running = next;
    readyPrograms.pop_front();

    asm_switch_thread(cur, next);

    interruptManager.setInterruptStatus(status);
}

void program_exit()
{
    PCB *thread = programManager.running;
    thread->status = ProgramStatus::DEAD;

    if (thread->pid)
    {
        programManager.schedule();
    }
    else
    {
        interruptManager.disableInterrupt();
        printf("halt\n");
        asm_halt();
    }
}

PCB *ProgramManager::allocatePCB()
{
    for (int i = 0; i < MAX_PROGRAM_AMOUNT; ++i)
    {
        if (!PCB_SET_STATUS[i])
        {
            PCB_SET_STATUS[i] = true;
            return (PCB *)((int)PCB_SET + PCB_SIZE * i);
        }
    }

    return nullptr;
}

void ProgramManager::releasePCB(PCB *program)
{
    int index = ((int)program - (int)PCB_SET) / PCB_SIZE;
    PCB_SET_STATUS[index] = false;
}

void ProgramManager::update_shortest_remain_time()
{
    bool status = interruptManager.getInterruptStatus();
    interruptManager.disableInterrupt();

    shortest_remain_time = 0x7fffffff;  // 初始化为最大整数
    shortest_program = nullptr;

    if (readyPrograms.size() == 0) {
        interruptManager.setInterruptStatus(status);
        return;
    }

    // 遍历就绪队列
    ListItem *item = readyPrograms.front();  // 使用front()获取第一个节点
    while (item) {  // 通过next指针遍历直到nullptr
        PCB *program = ListItem2PCB(item, tagInGeneralList);
        if (program->ticks < shortest_remain_time) {
            shortest_remain_time = program->ticks;
            shortest_program = program;
        }
        item = item->next;
    }

    interruptManager.setInterruptStatus(status);
}