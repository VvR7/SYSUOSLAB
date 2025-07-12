#include "memory.h"
#include "os_constant.h"
#include "stdlib.h"
#include "asm_utils.h"
#include "stdio.h"
#include "program.h"
#include "os_modules.h"


MemoryManager::MemoryManager()
{
    initialize();
}

void MemoryManager::initialize()
{
    allocate_lock.initialize();
    malloc_lock.initialize();
    free_lock.initialize();
    swap_lock.initialize();
    this->totalMemory = 0;
    this->totalMemory = getTotalMemory();
    swapAreaManager.initialize();
    // 预留的内存
    int usedMemory = 256 * PAGE_SIZE + 0x100000;
    if (this->totalMemory < usedMemory)
    {
        printf("memory is too small, halt.\n");
        asm_halt();
    }
    // 剩余的空闲的内存
    int freeMemory = this->totalMemory - usedMemory;

    int freePages = freeMemory / PAGE_SIZE;
    int kernelPages = freePages / 2;
    int userPages = freePages - kernelPages;
    int kernelphysicalpage=kernelPages;
    this->kernelPhysicalStartAddress = usedMemory;
    this->userPhysicalStartAddress = usedMemory + kernelphysicalpage * PAGE_SIZE;
    int kernelPhysicalStartAddress = usedMemory;
    int userPhysicalStartAddress = usedMemory + kernelphysicalpage * PAGE_SIZE;

    int kernelPhysicalBitMapStart = BITMAP_START_ADDRESS;
    int userPhysicalBitMapStart = kernelPhysicalBitMapStart + ceil(kernelphysicalpage, 8);
    int kernelVirtualBitMapStart = userPhysicalBitMapStart + ceil(userPages, 8);

    kernelPhysical.initialize(
        (char *)kernelPhysicalBitMapStart,
        kernelphysicalpage,
        kernelPhysicalStartAddress);

    userPhysical.initialize(
        (char *)userPhysicalBitMapStart,
        userPages,
        userPhysicalStartAddress);

    kernelVirtual.initialize(
        (char *)kernelVirtualBitMapStart,
        kernelPages,
        KERNEL_VIRTUAL_START);

    printf("total memory: %d bytes ( %d MB )\n",
           this->totalMemory,
           this->totalMemory / 1024 / 1024);

    printf("kernel pool\n"
           "    start address: 0x%x\n"
           "    total pages: %d ( %d MB )\n"
           "    bitmap start address: 0x%x\n",
           kernelPhysicalStartAddress,
           kernelphysicalpage, kernelphysicalpage * PAGE_SIZE / 1024 / 1024,
           kernelPhysicalBitMapStart);

    printf("user pool\n"
           "    start address: 0x%x\n"
           "    total pages: %d ( %d MB )\n"
           "    bit map start address: 0x%x\n",
           userPhysicalStartAddress,
           userPages, userPages * PAGE_SIZE / 1024 / 1024,
           userPhysicalBitMapStart);

    printf("kernel virtual pool\n"
           "    start address: 0x%x\n"
           "    total pages: %d  ( %d MB ) \n"
           "    bit map start address: 0x%x\n",
           KERNEL_VIRTUAL_START,
           userPages, kernelPages * PAGE_SIZE / 1024 / 1024,
           kernelVirtualBitMapStart);
}

int MemoryManager::allocatePhysicalPages(enum AddressPoolType type, const int count)
{
    int start = -1;

    if (type == AddressPoolType::KERNEL)
    {
        start = kernelPhysical.allocate(count);
    }
    else if (type == AddressPoolType::USER)
    {
        start = userPhysical.allocate(count);
    }

    return (start == -1) ? 0 : start;
}

void MemoryManager::releasePhysicalPages(enum AddressPoolType type, const int paddr, const int count)
{
    if (type == AddressPoolType::KERNEL)
    {
        kernelPhysical.release(paddr, count);
    }
    else if (type == AddressPoolType::USER)
    {

        userPhysical.release(paddr, count);
    }
}

int MemoryManager::getTotalMemory()
{

    if (!this->totalMemory)
    {
        int memory = *((int *)MEMORY_SIZE_ADDRESS);
        // ax寄存器保存的内容
        int low = memory & 0xffff;
        // bx寄存器保存的内容
        int high = (memory >> 16) & 0xffff;

        this->totalMemory = low * 1024 + high * 64 * 1024;
    }

    return this->totalMemory;
}

int MemoryManager::allocatePages(enum AddressPoolType type, const int count)
{
    allocate_lock.lock();
    if (type==AddressPoolType::USER) {
        int virtualAddress = allocateVirtualPages(AddressPoolType::USER, count);
        if (!virtualAddress)
        {
            allocate_lock.unlock();
            return 0;
        }
        allocate_lock.unlock();
        return virtualAddress;
    }
    // 第一步：从虚拟地址池中分配若干虚拟页
    int virtualAddress = allocateVirtualPages(AddressPoolType::KERNEL, count);
    if (!virtualAddress)
    {
        allocate_lock.unlock();
        return 0;
    }

    bool flag;
    int physicalPageAddress;
    int vaddress = virtualAddress;

    // 依次为每一个虚拟页指定物理页
    for (int i = 0; i < count; ++i, vaddress += PAGE_SIZE)
    {
        flag = false;
        // 第二步：从物理地址池中分配一个物理页
        physicalPageAddress = allocatePhysicalPages(AddressPoolType::KERNEL, 1);
        if (physicalPageAddress)
        {
            //printf("allocate physical page 0x%x\n", physicalPageAddress);
            
            // 第三步：为虚拟页建立页目录项和页表项，使虚拟页内的地址经过分页机制变换到物理页内。
            flag = connectPhysicalVirtualPage(vaddress, physicalPageAddress);
        }
        else
        {
            flag = false;
        }

        // 分配失败，释放前面已经分配的虚拟页和物理页表
        if (!flag)
        {
            // 前i个页表已经指定了物理页
            releasePages(AddressPoolType::KERNEL, virtualAddress, i);
            // 剩余的页表未指定物理页
            releaseVirtualPages(AddressPoolType::KERNEL, virtualAddress + i * PAGE_SIZE, count - i);
            allocate_lock.unlock();
            return 0;
        }
    }

    allocate_lock.unlock();
    return virtualAddress;
}

int MemoryManager::allocateVirtualPages(enum AddressPoolType type, const int count)
{
    int start = -1;

    if (type == AddressPoolType::KERNEL)
    {
        start = kernelVirtual.allocate(count);
    }
    else if (type == AddressPoolType::USER)
    {
        start = programManager.running->userVirtual.allocate(count);
    }

    return (start == -1) ? 0 : start;
}

bool MemoryManager::connectPhysicalVirtualPage(const int virtualAddress, const int physicalPageAddress)
{
    // 计算虚拟地址对应的页目录项和页表项
    int *pde = (int *)toPDE(virtualAddress);
    int *pte = (int *)toPTE(virtualAddress);

    // 页目录项无对应的页表，先分配一个页表
    if (!(*pde & 0x00000001))
    {
        // 从内核物理地址空间中分配一个页表
        int page = allocatePhysicalPages(AddressPoolType::KERNEL, 1);
        if (!page)
            return false;

        // 使页目录项指向页表
        *pde = page | 0x7;
        // 初始化页表
        char *pagePtr = (char *)(((int)pte) & 0xfffff000);
        memset(pagePtr, 0, PAGE_SIZE);
    }

    // 使页表项指向物理页
    *pte = physicalPageAddress | 0x7;

    return true;
}

int MemoryManager::toPDE(const int virtualAddress)
{
    return (0xfffff000 + (((virtualAddress & 0xffc00000) >> 22) * 4));
}

int MemoryManager::toPTE(const int virtualAddress)
{
    return (0xffc00000 + ((virtualAddress & 0xffc00000) >> 10) + (((virtualAddress & 0x003ff000) >> 12) * 4));
}
void MemoryManager::releasePages(enum AddressPoolType type, const int virtualAddress, const int count)
{
    int vaddr = virtualAddress;
    *((int *)vaddr)=0;  //为它调入物理页或者创建物理页
    int *pte;
    for (int i = 0; i < count; ++i, vaddr += PAGE_SIZE)
    {
        // 第一步，对每一个虚拟页，释放为其分配的物理页
        int paddr=vaddr2paddr(vaddr);
        int index=read_only_page_queue.find_page(paddr);
        if (index!=-1) {
            read_only_page_queue.pop_page(paddr);
        } 
        if (index==-1||read_only_page_queue.queue[index].count==0) {
            releasePhysicalPages(type, vaddr2paddr(vaddr), 1);
            // 设置页表项为不存在，防止释放后被再次使用
            pte = (int *)toPTE(vaddr);
            *pte = 0;
        }
    }

    // 第二步，释放虚拟页
    releaseVirtualPages(type, virtualAddress, count);
}

int MemoryManager::vaddr2paddr(int vaddr)
{
    int *pte = (int *)toPTE(vaddr);
    int page = (*pte) & 0xfffff000;
    int offset = vaddr & 0xfff;
    return (page + offset);
}

void MemoryManager::releaseVirtualPages(enum AddressPoolType type, const int vaddr, const int count)
{
    if (type == AddressPoolType::KERNEL)
    {
        kernelVirtual.release(vaddr, count);
    }
    else if (type == AddressPoolType::USER)
    {
        programManager.running->userVirtual.release(vaddr, count);
    }
}

static inline void flush_tlb_single(int addr) {
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}
void MemoryManager::swapOut(uint32 vaddr) {
    swap_lock.lock();
    int pagenum = swapAreaManager.allocateSwapPage();  //分配交换区页
    if (pagenum == -1) {
        swap_lock.unlock();
        printf("No swap space available\n");
        return;
    }
    swapAreaManager.swapOut(vaddr, pagenum);//写到外存
    int *pte = (int *)toPTE(vaddr);
    memset((void *)(vaddr&0xfffff000),0,PAGE_SIZE);
    flush_tlb_single(vaddr);
    *pte &=0xfffffffe;  //存在位为0
    *pte &=0x00000fff;  //31-12位清空
    *pte |=pagenum<<12;  //31-12位设为pagenum
    swap_lock.unlock();
}
int MemoryManager::physicalPageID(int paddr)
{
    return (paddr - kernelPhysicalStartAddress) / PAGE_SIZE;
}
int MemoryManager::virtualPageID(int vaddr)
{
    return (vaddr - KERNEL_VIRTUAL_START) / PAGE_SIZE;
}
int min(int a,int b)
{
    return a<b?a:b;
}
void *MemoryManager::malloc(int size)   //规定分配的空间必须连续
{
    malloc_lock.lock();
    if (size<0) {
        malloc_lock.unlock();
        return nullptr;
    }
    int minn=1e9;
    int index=-1;
    if (size<PAGE_SIZE) {
        for (int i=free_queue.front;i!=free_queue.rear;i=(i+1)%MAX_QUEUE_SIZE)
        {
            if (free_queue.queue[i].size>=size&&free_queue.queue[i].size<minn)
            {
                minn=free_queue.queue[i].size;
                index=i;
            }
        }
    }
    if (index!=-1) {
        int start=free_queue.queue[index].vaddr;
        free_queue.queue[index].vaddr+=size;
        free_queue.queue[index].size-=size;
        if (free_queue.queue[index].size==0) {
            free_queue.del(index);
        }
        memset((void *)start,0,size);
        malloc_lock.unlock();
        return (void *)start;
    }
    int start=allocatePages(AddressPoolType::USER,ceil(size,PAGE_SIZE));
    if (start==0) return nullptr;
    if (size%PAGE_SIZE!=0) {
        int pos=start+(ceil(size,PAGE_SIZE)-1)*PAGE_SIZE;
        pos+=size%PAGE_SIZE;
        free_queue.push(free_block(pos,PAGE_SIZE-size%PAGE_SIZE));
    }
    memset((void *)start,0,size);
    malloc_lock.unlock();
    return (void *)start;
}
void MemoryManager::free(void *ptr,int size)
{
    free_lock.lock();
    //若size小于一页:
    if (size<PAGE_SIZE) {
        memset((void *)ptr,0,size);
        int nextstart=(int)ptr+size;
        int lastend=(int)ptr-1;
        bool has_last=false;
        bool has_next=false;
        int next_index=-1;
        int last_index=-1;
        for (int i=free_queue.front;i!=free_queue.rear;i=(i+1)%MAX_QUEUE_SIZE)
        {
            if (free_queue.queue[i].vaddr==nextstart) {
                free_queue.queue[i].vaddr=(int)ptr;
                free_queue.queue[i].size+=size;
                if (free_queue.queue[i].size==PAGE_SIZE) {
                    releasePages(AddressPoolType::USER,free_queue.queue[i].vaddr,1);
                    free_queue.del(i);
                }
                has_next=true;
                next_index=i;
            } 
            if (free_queue.queue[i].vaddr+free_queue.queue[i].size-1==lastend) {
                has_last=true;
                last_index=i;
            }
        }
        if (has_next&&!has_last) return;  //有后无前
        if (has_last&&!has_next) {  //有前无后
            free_queue.queue[last_index].size+=size;
            if (free_queue.queue[last_index].size==PAGE_SIZE) {
                releasePages(AddressPoolType::USER,free_queue.queue[last_index].vaddr,1);
                free_queue.del(last_index);
            }
        } else 
        if (has_next&&has_last) {  //前后都有
            free_queue.queue[last_index].size+=free_queue.queue[next_index].size;
            free_queue.del(next_index);
            for (int i=free_queue.front;i!=free_queue.rear;i=(i+1)%MAX_QUEUE_SIZE) {
                if (free_queue.queue[i].size==PAGE_SIZE) {
                    releasePages(AddressPoolType::USER,free_queue.queue[i].vaddr,1);
                    free_queue.del(i);
                    break;
                }
            }
        } else free_queue.push(free_block((int)ptr,size));  //前后都无
        free_lock.unlock();
        return;
    }
    //若size大于一页，则ptr一定位于一个页的开始
    while (size)
    {
        if (size>=PAGE_SIZE) { 
            releasePages(AddressPoolType::USER,(int)ptr,1);
            size-=PAGE_SIZE;
            ptr+=PAGE_SIZE;
        } else {
            free_lock.unlock();
            free(ptr,size);
            return;
        }
    }
    free_lock.unlock();
}
uint32 MemoryManager::exchangepage(int vaddr)
{
    uint32 physicalPageAddress;
    // if (vaddr>=KERNEL_VIRTUAL_START) {    //内核态
    //     if (KernelphysicalPageQueue.empty()) {
    //         asm_halt();
    //         }
    //         item u=KernelphysicalPageQueue.getFront();
    //         KernelphysicalPageQueue.pop();
    //         swapOut(u.virtualAddress);
    //         releasePhysicalPages(AddressPoolType::KERNEL, u.physicalAddress, 1);
    //         physicalPageAddress=allocatePhysicalPages(AddressPoolType::KERNEL, 1);
    //         flush_tlb_single(u.virtualAddress);
    //     }   
    //     else 
    if (vaddr>=USER_VADDR_START&&vaddr<0xc0000000) {  //用户态
            if (UserphysicalPageQueue.empty()) {
                asm_halt();
            }
            item u=UserphysicalPageQueue.getFront();
            UserphysicalPageQueue.pop();
            swapOut(u.virtualAddress);
            releasePhysicalPages(AddressPoolType::USER, u.physicalAddress, 1);
            physicalPageAddress=allocatePhysicalPages(AddressPoolType::USER, 1);
            flush_tlb_single(u.virtualAddress);

        }    
    return physicalPageAddress;
}
void MemoryManager::COW_page_default_management(int vaddr)
{
    int *pte=(int *)toPTE(vaddr);
    int paddr=*pte&0xfffff000;
    int index=read_only_page_queue.find_page(paddr);
    if (read_only_page_queue.queue[index].count<=1) {
        *pte|=0x2;
        return;
    } else {
        read_only_page_queue.queue[index].count--;
    }
    char *buffer = (char *)malloc(PAGE_SIZE);
    memcpy((char *)vaddr,buffer,PAGE_SIZE);
    //printf("buffer:%s\n",buffer);
    int physicalPageAddress;
    physicalPageAddress=allocatePhysicalPages(AddressPoolType::USER,1);
    if (physicalPageAddress == 0) {
        physicalPageAddress=exchangepage(vaddr);
    }
    UserphysicalPageQueue.push(item(vaddr,physicalPageAddress));
    *pte|=0x2;  //设置为可写
    bool flag=connectPhysicalVirtualPage(vaddr,physicalPageAddress);
    if (!flag) {
        printf("connect with physical page failed\n");
        asm_halt();
    }
    flush_tlb_single(vaddr);
    memcpy(buffer,(char *)vaddr,PAGE_SIZE);
    free(buffer,PAGE_SIZE);
}
