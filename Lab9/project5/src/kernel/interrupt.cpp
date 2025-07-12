#include "interrupt.h"
#include "os_type.h"
#include "os_constant.h"
#include "asm_utils.h"
#include "stdio.h"
#include "os_modules.h"
#include "program.h"

int times = 0;
extern ProgramManager programManager;
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
    programManager.releaseZombie();
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

extern MemoryManager memoryManager;
static inline void flush_tlb_single(int addr) {
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}
uint32 read_cr2() {
    uint32 value;
    asm volatile("mov %%cr2, %0" : "=r"(value));
    return value;
}
bool is_valid(uint32 vaddr)
{
    uint32 page_addr=vaddr & 0xfffff000;
    if (vaddr >= KERNEL_VIRTUAL_START) {
        int page_index = (page_addr - memoryManager.kernelVirtual.startAddress) / 4096;
        if (page_index >= 0 && page_index < memoryManager.kernelVirtual.resources.length) {
            // 检查该页是否已分配（即位图中该位是否为1）
            return memoryManager.kernelVirtual.resources.get(page_index);
        }
    } else if (vaddr>=USER_VADDR_START&&vaddr<0xc0000000){
        PCB* cur=programManager.running;
        int page_index = (page_addr - cur->userVirtual.startAddress) / 4096;
        if (page_index >= 0 && page_index < cur->userVirtual.resources.length) {
            return cur->userVirtual.resources.get(page_index);
        }
    } 
    return true;    
}
extern "C" void c_page_interrupt_handler(uint32 error_code)
{
    uint32 cr2=read_cr2();
    cr2=cr2&0xfffff000;
    // 分析错误码
    bool page_present = error_code & 0x1;     // 位0: 页是否存在    为0则是缺页错误   为1则是页保护错误
    bool write_access = error_code & 0x2;     // 位1: 是否为写访问
    if (page_present && write_access) {
        printf("COW page default on virtual address :0x%x\n",cr2);
        memoryManager.COW_page_default_management(cr2);
        return;
    }
    //printf("page fault on virtual address :0x%x\n",cr2);
    if (!is_valid(cr2)) {
        printf("Illegal memory access at 0x%x, process terminated\n", cr2);
        asm_halt();
        return;
    }
    // 合法地址，为其分配物理页
    int physicalPageAddress;
    if (cr2 >= 0xC0000000) {
        physicalPageAddress = memoryManager.allocatePhysicalPages(AddressPoolType::KERNEL, 1);
    } else {
        physicalPageAddress = memoryManager.allocatePhysicalPages(AddressPoolType::USER, 1);
    }
    if (physicalPageAddress == 0) {
        physicalPageAddress=memoryManager.exchangepage(cr2);
    }
    // if (cr2>=KERNEL_VIRTUAL_START) {
    //     memoryManager.KernelphysicalPageQueue.push(item(cr2,physicalPageAddress));
    // }
    // else 
    if (cr2>=USER_VADDR_START&&cr2<0xc0000000) {
        memoryManager.UserphysicalPageQueue.push(item(cr2,physicalPageAddress));   
    }
    int *pte=(int *)memoryManager.toPTE(cr2);
    int *pde=(int *)memoryManager.toPDE(cr2);
    int pagenum=0;
    if (!(*pde & 0x00000001)&&!page_present) pagenum=0;   //无对应页表
        else pagenum=(*pte)>>12;   //有对应页表
    bool flag=true;
    flag=memoryManager.connectPhysicalVirtualPage(cr2, physicalPageAddress);  
    if (pagenum!=0) {  
        bool read_only=((*pte)&0x2)>0?false:true;
        if (read_only) {   //还要判断是否换入只读页
            *pte&=0xfffffffd;  //设置为只读
        }
        memoryManager.swapAreaManager.swapIn(cr2,pagenum);
        printf("swap in page from disk\n");
    }
    if (!flag) {
        printf("connect with physical page failed\n");
        asm_halt();
    }

}