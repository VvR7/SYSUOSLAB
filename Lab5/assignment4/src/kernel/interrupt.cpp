#include "interrupt.h"
#include "os_type.h"
#include "os_constant.h"
#include "asm_utils.h"
#include "stdio.h"
#include "os_modules.h"
#include "program.h"

int times = 0;

// 声明外部变量和函数
extern int system_ticks;
extern bool second_created;
extern bool third_created;
extern void second_thread(void *);
extern void third_thread(void *);
extern void first_thread(void *);
InterruptManager::InterruptManager()
{
    initialize();
}

void InterruptManager::initialize()
{
    // 初始化中断计数变量
    times = 0;
    
    // 初始化IDT
    IDT = (uint32 *)IDT_START_ADDRESS;
    asm_lidt(IDT_START_ADDRESS, 256 * 8 - 1);

    for (uint i = 0; i < 256; ++i)
    {
        setInterruptDescriptor(i, (uint32)asm_unhandled_interrupt, 0);
    }

    // 初始化8259A芯片
    initialize8259A();
}

void InterruptManager::setInterruptDescriptor(uint32 index, uint32 address, byte DPL)
{
    IDT[index * 2] = (CODE_SELECTOR << 16) | (address & 0xffff);
    IDT[index * 2 + 1] = (address & 0xffff0000) | (0x1 << 15) | (DPL << 13) | (0xe << 8);
}

void InterruptManager::initialize8259A()
{
    // ICW 1
    asm_out_port(0x20, 0x11);
    asm_out_port(0xa0, 0x11);
    // ICW 2
    IRQ0_8259A_MASTER = 0x20;
    IRQ0_8259A_SLAVE = 0x28;
    asm_out_port(0x21, IRQ0_8259A_MASTER);
    asm_out_port(0xa1, IRQ0_8259A_SLAVE);
    // ICW 3
    asm_out_port(0x21, 4);
    asm_out_port(0xa1, 2);
    // ICW 4
    asm_out_port(0x21, 1);
    asm_out_port(0xa1, 1);

    // OCW 1 屏蔽主片所有中断，但主片的IRQ2需要开启
    asm_out_port(0x21, 0xfb);
    // OCW 1 屏蔽从片所有中断
    asm_out_port(0xa1, 0xff);
}

void InterruptManager::enableTimeInterrupt()
{
    uint8 value;
    // 读入主片OCW
    asm_in_port(0x21, &value);
    // 开启主片时钟中断，置0开启
    value = value & 0xfe;
    asm_out_port(0x21, value);
}

void InterruptManager::disableTimeInterrupt()
{
    uint8 value;
    asm_in_port(0x21, &value);
    // 关闭时钟中断，置1关闭
    value = value | 0x01;
    asm_out_port(0x21, value);
}

void InterruptManager::setTimeInterrupt(void *handler)
{
    setInterruptDescriptor(IRQ0_8259A_MASTER, (uint32)handler, 0);
}

// 中断处理函数
extern "C" void c_time_interrupt_handler()
{
    // 增加系统时钟计数
    system_ticks++;
    if (system_ticks<=5)printf("------------system_ticks: %d------------------\n", system_ticks);
    // 在特定时间创建新线程
    if (system_ticks == 0) {
        printf("System_ticks: %d [Time interrupt] Create first thread\n", system_ticks);
        programManager.executeThread(first_thread, nullptr, "first thread", 8);
    }
    if (system_ticks == 3 ) {
        printf("System_ticks: %d [Time interrupt] Create second thread\n", system_ticks);
        programManager.executeThread(second_thread, nullptr, "second thread", 4);
    }
    else if (system_ticks == 4 ) {
        printf("System_ticks: %d [Time interrupt] Create third thread\n", system_ticks);
        programManager.executeThread(third_thread, nullptr, "third thread", 2);
    }    
    PCB *cur = programManager.running;
    if (cur->ticks) {
        --cur->ticks;
        if (system_ticks<=5) printf("pid:%d curticks: %d\n", cur->pid, cur->ticks);
        ++cur->ticksPassedBy;
        programManager.schedule();
    }
}

void InterruptManager::enableInterrupt()
{
    asm_enable_interrupt();
}

void InterruptManager::disableInterrupt()
{
    asm_disable_interrupt();
}

bool InterruptManager::getInterruptStatus()
{
    return asm_interrupt_status() ? true : false;
}

// 设置中断状态
void InterruptManager::setInterruptStatus(bool status)
{
    if (status)
    {
        enableInterrupt();
    }
    else
    {
        disableInterrupt();
    }
}

