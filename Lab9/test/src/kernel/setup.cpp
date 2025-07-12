#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"
#include "memory.h"
#include "syscall.h"
#include "tss.h"

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
    printf("first process\n");
    char *p=(char *)malloc(4097);
    printf("0x%x\n",p);
    char *p1=(char *)malloc(1024);
    printf("0x%x\n",p1);
    char *p2=(char *)malloc(1024);
    printf("0x%x\n",p2);
    printf("number of free blocks: %d\n",memoryManager.free_queue.size());
    free(p,4097);
    printf("number of free blocks: %d\n",memoryManager.free_queue.size());
    free(p1,1024);
    printf("number of free blocks: %d\n",memoryManager.free_queue.size());
    free(p2,1024);
    printf("number of free blocks: %d\n",memoryManager.free_queue.size());
}
void second_process()
{
    printf("second process\n");
    int retval;
    int x=1;
    int pid=fork();
    int pid2=fork();
    if (pid==0) {
        printf("child process\n");
        x=2;
    } else {
        printf("parent process\n");
        wait(&retval);
        printf("%d\n",x);
    }
}
// void second_thread(void *arg)
// {
//     printf("thread exit\n");
//     //exit(0);
// }
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
    
    // char *p=(char *)memoryManager.allocatePages(AddressPoolType::KERNEL,8);
    // char num='a';
    // for (int i=0;i<8;i++)
    // {
    //     p[i*PAGE_SIZE]=num;
    //     num++;
    //     flush_tlb_single((int)p+i*PAGE_SIZE);
    // }
    // for (int i=3;i<8;i++)
    //   printf("%c\n",p[i*PAGE_SIZE]);
    // for (int i=0;i<3;i++)
    // {
    //     printf("%c\n",p[i*PAGE_SIZE]);
    //     flush_tlb_single((int)p+i*PAGE_SIZE);
    // }
    //int pid=programManager.executeProcess((const char *)first_process,1);
    int pid2=programManager.executeProcess((const char *)second_process,1);
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
