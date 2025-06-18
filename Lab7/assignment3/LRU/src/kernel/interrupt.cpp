#include "interrupt.h"
#include "os_type.h"
#include "os_constant.h"
#include "asm_utils.h"
#include "stdio.h"
#include "os_modules.h"
#include "program.h"
#include "memory.h"
extern MemoryManager memoryManager;
int times = 0;
static inline void flush_tlb_single(int addr) {
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}
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
        if (i!=14)
            setInterruptDescriptor(i, (uint32)asm_unhandled_interrupt, 0);
    }
    setInterruptDescriptor(14, (uint32)asm_page_fault_handler, 0);
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
    PCB *cur = programManager.running;

    if (cur->ticks)
    {
        --cur->ticks;
        ++cur->ticksPassedBy;
    }
    else
    {
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
uint32 read_cr2() {
    uint32 value;
    asm volatile("mov %%cr2, %0" : "=r"(value));
    return value;
}
extern "C" void c_page_interrupt_handler()
{
    uint32 cr2=read_cr2();
    printf("page fault on virtual address :0x%x\n",cr2);
    bool is_valid = false;
    
    // 计算地址所在的页面
    uint32 page_addr = cr2 & 0xFFFFF000;
    
    // 检查是否是内核地址空间（3GB以上）
    if (cr2 >= 0xC0000000) {
        // 内核地址空间，检查该地址是否在内核虚拟地址池的已分配范围内
        int page_index = (page_addr - memoryManager.kernelVirtual.startAddress) / 4096;
        if (page_index >= 0 && page_index < memoryManager.kernelVirtual.resources.length) {
            // 检查该页是否已分配（即位图中该位是否为1）
            is_valid = memoryManager.kernelVirtual.resources.get(page_index);
        }
    } else {
        is_valid = false;
    }
    
    if (!is_valid) {
        printf("Illegal memory access at 0x%x, process terminated\n", cr2);
        asm_halt();
        return;
    }
    
    // 合法地址，为其分配物理页
    int physicalPageAddress;
    bool flag=true;
    physicalPageAddress = memoryManager.allocatePhysicalPages(AddressPoolType::KERNEL, 1);
    if (physicalPageAddress == 0) {
        flag=false;
    }  

    if (!flag) {
        if (memoryManager.physicalPageQueue.empty()) {
            asm_halt();
        } 
        // int index=memoryManager.FindExchangePage();
        // item u=memoryManager.physicalPageQueue.queue[index];
        // //删除该物理页框和清空页表
        // memoryManager.releasePhysicalPages(AddressPoolType::KERNEL, u.physicalAddress, 1);
        // int *pte=(int *)memoryManager.toPTE(u.virtualAddress);
        // *pte=0;
        // physicalPageAddress=memoryManager.allocatePhysicalPages(AddressPoolType::KERNEL, 1);
        // item v(cr2,physicalPageAddress);
        // memoryManager.physicalPageQueue.queue[index]=v;
        // memoryManager.physicalPageQueue.cur=(index+1)%MAX_QUEUE_SIZE;
        // if (memoryManager.physicalPageQueue.cur==memoryManager.physicalPageQueue.rear) memoryManager.physicalPageQueue.cur=memoryManager.physicalPageQueue.front;
        // flush_tlb_single(u.virtualAddress);
        int index=memoryManager.FindExchangePage();
        item u=memoryManager.physicalPageQueue.queue[index];
        memoryManager.physicalPageQueue.del(index);
        memoryManager.releasePhysicalPages(AddressPoolType::KERNEL, u.physicalAddress, 1);
        int *pte=(int *)memoryManager.toPTE(u.virtualAddress);
        *pte=0;
        physicalPageAddress=memoryManager.allocatePhysicalPages(AddressPoolType::KERNEL, 1);
        item v(cr2,physicalPageAddress);
        memoryManager.physicalPageQueue.push(v);
        flush_tlb_single(u.virtualAddress);
    } else {   //物理地址没分配满
        memoryManager.physicalPageQueue.push(item(cr2,physicalPageAddress));
    }
    flag=memoryManager.connectPhysicalVirtualPage(cr2, physicalPageAddress);
    if (!flag) {
        printf("connect with physical page failed\n");
        asm_halt();
    }
    printf("Connect virtual Page:%d with physical Page:%d\n",memoryManager.virtualPageID(cr2),memoryManager.physicalPageID(physicalPageAddress));
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

