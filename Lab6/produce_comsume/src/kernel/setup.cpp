#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"
#include "program.h"
#include "thread.h"
#include "sync.h"

// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;
// 程序管理器
ProgramManager programManager;

// 信号量
Semaphore semaphore;

// 缓冲区大小
#define BUFFER_SIZE 3

// 缓冲区
int buffer[BUFFER_SIZE];
// 生产者放入位置
int in = 0;
// 消费者取出位置
int out = 0;
// 已生产的总数
int produced = 0;
// 已消费的总数
int consumed = 0;

// 生产者线程
void producer(void *arg)
{
    int item;
    int delay;
    
    for (int i = 0; i < 10; i++) {
        // 生产物品
        item = i + 1;
        
        // 获取空槽位
        semaphore.P_empty();
        // 获取互斥锁
        semaphore.P_mutex();
        
        // 放入缓冲区
        buffer[in] = item;
        printf("Producer %d Position %d, has produced %d \n", item, in, ++produced);
        in = (in + 1) % BUFFER_SIZE;
        
        // 释放互斥锁
        semaphore.V_mutex();
        // 增加满槽位
        semaphore.V_full();
        
        // 模拟生产耗时
        delay = 0x1ffffff;
        while (delay--);
    }
}

// 消费者线程
void consumer(void *arg)
{
    int item;
    int delay;
    
    for (int i = 0; i < 10; i++) {
        // 获取满槽位
        semaphore.P_full();
        // 获取互斥锁
        semaphore.P_mutex();
        
        // 从缓冲区取出
        item = buffer[out];
        printf("Comsumer %d Position %d, has consumed %d \n", item, out, ++consumed);
        out = (out + 1) % BUFFER_SIZE;
        
        // 释放互斥锁
        semaphore.V_mutex();
        // 增加空槽位
        semaphore.V_empty();
        
        // 模拟消费耗时
        delay = 0x2ffffff;
        while (delay--);
    }
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

    // 初始化缓冲区和计数
    for (int i = 0; i < BUFFER_SIZE; i++) {
        buffer[i] = 0;
    }
    in = 0;
    out = 0;
    produced = 0;
    consumed = 0;
    
    // 初始化信号量，设置缓冲区大小
    semaphore.initialize(BUFFER_SIZE);
    

    printf("buffer size: %d\n", BUFFER_SIZE);
    
    // 创建生产者线程和消费者线程
    programManager.executeThread(consumer, nullptr, "Comsumer", 1);
    programManager.executeThread(producer, nullptr, "Producer", 1);

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
