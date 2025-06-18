#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"
#define BUFFER_SIZE 20
// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;
char buffer[20];
// 信号量
Semaphore semaphore;

int read_count=0;
void Reader_thread(void *arg)
{
    int id=(int)arg;
    semaphore.P_mutex();
    if (read_count==0)
    {
        semaphore.P_rw();
    }
    read_count++;
    semaphore.V_mutex();
    printf("reader thread %d is reading\n",id);
    printf("buffer: %s\n",buffer);
    programManager.schedule();
    semaphore.P_mutex();
    read_count--;
    if (read_count==0)
    {
        semaphore.V_rw();
    }
    semaphore.V_mutex();
}
void Writer_thread(void *arg)
{
    int id=(int)arg;
    semaphore.P_rw();
    programManager.schedule();
    printf("writer thread %d is writing\n",id);
    char str[]="Writer:";
    for (int i=0;i<7;i++)
    {
        buffer[i]=str[i];
    }
    buffer[7]='0'+id;   
    printf("buffer: %s\n",buffer);
    semaphore.V_rw();
}
void first_thread(void *arg)
{
    // 第1个线程不可以返回
    stdio.moveCursor(0);
    for (int i = 0; i < 25 * 80; ++i)
    {
        stdio.print(' ');
    }
    stdio.moveCursor(0);

   
    char str[]="Empty";
    for (int i=0;i<5;i++)
    {
        buffer[i]=str[i];
    }
    // 初始化信号量，设置缓冲区大小
    semaphore.initialize();
    

    
    for (int i=0;i<3;i++)
    {
        programManager.executeThread(Reader_thread, (void *)i, "Reader", 1000);
        programManager.executeThread(Writer_thread, (void *)i, "Writer", 1000);
    }

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
