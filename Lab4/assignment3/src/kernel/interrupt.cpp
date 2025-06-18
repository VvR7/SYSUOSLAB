#include "interrupt.h"
#include "os_type.h"
#include "os_constant.h"
#include "asm_utils.h"
#include "stdio.h"
#include "os_modules.h"

int times = 0;

InterruptManager::InterruptManager()
{
    initialize();
}

void InterruptManager::initialize()
{
    // 初始化IDT
    IDT = (uint32 *)IDT_START_ADDRESS;
    asm_lidt(IDT_START_ADDRESS, 256 * 8 - 1);
    for (uint i = 0; i < 256; ++i)
    {
        setInterruptDescriptor(i, (uint32)asm_unhandled_interrupt, 0);
    }
    setInterruptDescriptor(14,(uint32)asm_page_fault_handler,0);//页面错误的中断向量号14
}

void InterruptManager::setInterruptDescriptor(uint32 index, uint32 address, byte DPL)
{
    IDT[index * 2] = (CODE_SELECTOR << 16) | (address & 0xffff);
    IDT[index * 2 + 1] = (address & 0xffff0000) | (0x1 << 15) | (DPL << 13) | (0xe << 8);
}

extern "C" void c_page_interrupt_handler()
{
    for (int i=0;i<25;i++)
      for (int j=0;j<80;j++)
        stdio.print(i,j,' ',0x07);
    stdio.moveCursor(0);

    char str[]="#PF Page fault";
    for (char*c=str;*c;c++)
    {
        stdio.print(0,c-str,*c,0x03);
    }
}
