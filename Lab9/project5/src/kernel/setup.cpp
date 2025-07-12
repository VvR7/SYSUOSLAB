#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"
#include "memory.h"
#include "syscall.h"
#include "tss.h"
#include "stdlib.h"
// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;
// 内存管理器
MemoryManager memoryManager;
// 系统调用
SystemService systemService;
// Task State Segment
TSS tss;

int syscall_0(int first, int second, int third, int forth, int fifth)
{
    printf("systerm call 0: %d, %d, %d, %d, %d\n",
           first, second, third, forth, fifth);
    return first + second + third + forth + fifth;
}

void first_process()
{
    int pid1=fork();
    int pid2=fork();
}
uint32 getpaddr(uint32 vaddr)
{
    int *pte=(int *)memoryManager.toPTE(vaddr);
    return *pte&0xfffff000;
}
void second_process()
{
    malloc(0x3b7*4096);
    //从0x08400000开始，父子共享
    int retval;
    char *p=(char *)malloc(15);
    strcpy("hello",p);
    int pid=fork();
    if (pid==0) {
        uint32 paddr=getpaddr((int)p);
        printf("child before COW: vaddr:%x paddr:%x %s\n",p,paddr,p);
        //父进程已经写时复制，子进程不复制
        strcpy("sysu",p);
        paddr=getpaddr((int)p);
        printf("child after COW: vaddr:%x paddr:%x %s\n",p,paddr,p);
        //再写，这次不复制
        strcpy("good afternoon",p);
        paddr=getpaddr((int)p);
        printf("child after COW2: vaddr:%x paddr:%x %s\n",p,paddr,p);
        free(p,15);
    } else {
        uint32 paddr=getpaddr((int)p);
        printf("parent before COW: vaddr:%x paddr:%x %s\n",p,paddr,p);
        //写时复制
        strcpy(" world",p+5);
        paddr=getpaddr((int)p);
        printf("parent after COW: vaddr:%x paddr:%x %s\n",p,paddr,p);
        //再写，这次不复制
        strcpy("good morning",p);
        paddr=getpaddr((int)p);
        printf("parent after COW2: vaddr:%x paddr:%x %s\n",p,paddr,p);
        printf("\n\n\n");
        free(p,15);
    }
    asm_halt();
}
int getcurpid()
{
    return programManager.running->pid;
}
void third_process()
{
    malloc(0x3b7*4096);
    int retval;
    char *p=(char *)malloc(15);
    strcpy("hello",p);
    int pid=fork();
    if (pid==0) {
        printf("pid: %d before cow paddr: %x content: %s\n",getcurpid(),getpaddr((int)p),p);
        strcpy("sysu",p);
        printf("pid: %d paddr: %x content: %s\n",getcurpid(),getpaddr((int)p),p);
        printf("\n");
    } else {
        int pid2=fork();
        if (pid2==0) {
            printf("pid: %d before cow paddr: %x content: %s\n",getcurpid(),getpaddr((int)p),p);
            strcpy(" world",p+5);
            printf("pid: %d paddr: %x content: %s\n",getcurpid(),getpaddr((int)p),p);
            printf("\n");
        } else {
            // wait(&retval);
            // wait(&retval);
            printf("pid: %d before cow paddr: %x content: %s\n",getcurpid(),getpaddr((int)p),p);
            strcpy(" good morning",p);
            printf("pid: %d paddr: %x content: %s\n",getcurpid(),getpaddr((int)p),p);
            printf("\n");
        }
    }
    asm_halt();
}
static inline void flush_tlb_single(int addr) {
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
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

    //int pid=programManager.executeProcess((const char *)first_process,1);
    //int pid2=programManager.executeProcess((const char *)second_process,1);
    int pid3=programManager.executeProcess((const char *)third_process,1);
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

    // 初始化系统调用
    systemService.initialize();
    // 设置0号系统调用
    systemService.setSystemCall(0, (int)syscall_0);
    // 设置1号系统调用
    systemService.setSystemCall(1, (int)syscall_write);
    // 设置2号系统调用
    systemService.setSystemCall(2, (int)syscall_fork);
    // 设置3号系统调用
    systemService.setSystemCall(3, (int)syscall_exit);
    // 设置4号系统调用
    systemService.setSystemCall(4, (int)syscall_wait);
    // 设置5号系统调用
    systemService.setSystemCall(5, (int)syscall_malloc);
    // 设置6号系统调用
    systemService.setSystemCall(6, (int)syscall_free);
    // 内存管理器
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
    firstThread->status = ProgramStatus::RUNNING;
    programManager.readyPrograms.pop_front();
    programManager.running = firstThread;
    asm_switch_thread(0, firstThread);

    asm_halt();
}
