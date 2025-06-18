#include "asm_utils.h"
#include "interrupt.h"
#include "stdio.h"

// 屏幕IO处理器
STDIO stdio;
// 中断管理器
InterruptManager interruptManager;


extern "C" void setup_kernel()
{
    // 中断处理部件
    interruptManager.initialize();
    // 屏幕IO处理部件
    stdio.initialize();
    interruptManager.enableTimeInterrupt();
    interruptManager.setTimeInterrupt((void *)asm_time_interrupt_handler);
    printf("-123456: %f\n",-123.456);
    printf("-123.4567: %.2f\n",-123.4567);
    printf("-123.7654321: %e\n",-123.7654321);
    //uint a = 1 / 0;
    asm_halt();
}
