#include "disk.h"
#include "asm_utils.h"
#include "stdio.h"
#include "sync.h"
// 全局磁盘IO实例
DiskIO diskIO;
SpinLock swapLock;
DiskIO::DiskIO()
{
    // 构造函数，可以在这里进行初始化
}



bool DiskIO::readSector(uint32 lba, void* buffer)
{
    if (!buffer) {
        printf("Invalid buffer for disk read\n");
        return false;
    }
    
    // 检查LBA是否有效
    if (lba >= TOTAL_SECTORS) {
        printf("Invalid LBA: %d (max: %d)\n", lba, TOTAL_SECTORS - 1);
        return false;
    }
    
    uint8 status;
    uint16* wordBuffer = (uint16*)buffer;
    
    // 等待磁盘就绪
    int timeout = 10000;
    do {
        asm_in_port(ATA_STATUS_PORT, &status);
        timeout--;
        if (timeout <= 0) {
            printf("Disk read timeout waiting for ready\n");
            return false;
        }
    } while (status & ATA_STATUS_BSY);
    
    // 设置驱动器和LBA高4位 (LBA模式，主驱动器)
    asm_out_port(ATA_DRIVE_HEAD_PORT, 0xE0 | ((lba >> 24) & 0x0F));
    
    // 设置扇区数量为1
    asm_out_port(ATA_SECTOR_COUNT_PORT, 1);
    
    // 设置LBA地址
    asm_out_port(ATA_LBA_LOW_PORT, lba & 0xFF);
    asm_out_port(ATA_LBA_MID_PORT, (lba >> 8) & 0xFF);
    asm_out_port(ATA_LBA_HIGH_PORT, (lba >> 16) & 0xFF);
    
    // 发送读扇区命令
    asm_out_port(ATA_COMMAND_PORT, ATA_CMD_READ_SECTORS);
    
    // 等待数据准备好
    timeout = 10000;
    do {
        asm_in_port(ATA_STATUS_PORT, &status);
        timeout--;
        if (timeout <= 0) {
            printf("Disk read timeout waiting for data\n");
            return false;
        }
    } while (!(status & ATA_STATUS_DRQ) && !(status & ATA_STATUS_ERR));
    
    // 检查错误
    if (status & ATA_STATUS_ERR) {
        printf("Disk read error\n");
        return false;
    }
    
    // 读取数据 (512字节 = 256个16位字)
    for (int i = 0; i < 256; i++) {
        asm_in_port_word(ATA_DATA_PORT, &wordBuffer[i]);
    }
    
    //printf("Successfully read sector %d\n", lba);
    return true;
}

bool DiskIO::writeSector(uint32 lba, const void* buffer)
{
    if (!buffer) {
        printf("Invalid buffer for disk write\n");
        return false;
    }
    
    // 检查LBA是否有效
    if (lba >= TOTAL_SECTORS) {
        printf("Invalid LBA: %d (max: %d)\n", lba, TOTAL_SECTORS - 1);
        return false;
    }
    
    // 检查是否试图写入系统关键区域
    if (lba < SWAP_START_SECTOR) {
        printf("Warning: Writing to system area (LBA %d)\n", lba);
    }
    
    uint8 status;
    const uint16* wordBuffer = (const uint16*)buffer;
    
    // 等待磁盘就绪
    int timeout = 10000;
    do {
        asm_in_port(ATA_STATUS_PORT, &status);
        timeout--;
        if (timeout <= 0) {
            printf("Disk write timeout waiting for ready\n");
            return false;
        }
    } while (status & ATA_STATUS_BSY);
    
    // 设置驱动器和LBA高4位 (LBA模式，主驱动器)
    asm_out_port(ATA_DRIVE_HEAD_PORT, 0xE0 | ((lba >> 24) & 0x0F));
    
    // 设置扇区数量为1
    asm_out_port(ATA_SECTOR_COUNT_PORT, 1);
    
    // 设置LBA地址
    asm_out_port(ATA_LBA_LOW_PORT, lba & 0xFF);
    asm_out_port(ATA_LBA_MID_PORT, (lba >> 8) & 0xFF);
    asm_out_port(ATA_LBA_HIGH_PORT, (lba >> 16) & 0xFF);
    
    // 发送写扇区命令
    asm_out_port(ATA_COMMAND_PORT, ATA_CMD_WRITE_SECTORS);
    
    // 等待数据请求
    timeout = 10000;
    do {
        asm_in_port(ATA_STATUS_PORT, &status);
        timeout--;
        if (timeout <= 0) {
            printf("Disk write timeout waiting for data request\n");
            return false;
        }
    } while (!(status & ATA_STATUS_DRQ) && !(status & ATA_STATUS_ERR));
    
    // 检查错误
    if (status & ATA_STATUS_ERR) {
        printf("Disk write error\n");
        return false;
    }
    
    // 写入数据 (512字节 = 256个16位字)
    for (int i = 0; i < 256; i++) {
        asm_out_port_word(ATA_DATA_PORT, wordBuffer[i]);
    }
    
    // 等待写入完成
    timeout = 10000;
    do {
        asm_in_port(ATA_STATUS_PORT, &status);
        timeout--;
        if (timeout <= 0) {
            printf("Disk write timeout waiting for completion\n");
            return false;
        }
    } while (status & ATA_STATUS_BSY);
    
    // 检查最终状态
    if (status & ATA_STATUS_ERR) {
        printf("Disk write completion error\n");
        return false;
    }
    
    //printf("Successfully wrote sector %d\n", lba);
    return true;
}

bool DiskIO::isSwapSector(uint32 lba)
{
    return (lba >= SWAP_START_SECTOR && lba < SWAP_START_SECTOR + SWAP_SECTOR_COUNT);
}

uint32 DiskIO::getSwapStartSector()
{
    return SWAP_START_SECTOR;
}

uint32 DiskIO::getSwapSectorCount()
{
    return SWAP_SECTOR_COUNT;
}



// class SwapAreaManager
// {
//     int totalSwapPages;
//     int usedSwapPages;
//     int freeSwapPages;
//     bool swapPages[SWAP_SECTOR_COUNT/8];  //管理交换区每一页是否被分配
// public:
//     SwapAreaManager();
//     void initialize();
//     //页的换出
//     void swapOut(uint32 vaddr,uint32 pagenum);  //vaddr:换到外存的页的起始虚拟地址，pagenum:交换区的编号
//     //页的换入
//     void swapIn(uint32 vaddr,uint32 pagenum);
//     int pagenum_to_lba(uint32 pagenum); //页号转换为LBA编号(起始连续八个)
//     int allocateSwapPage(); //分配一个交换区页，返回pagenum
// };
SwapAreaManager::SwapAreaManager() {
    //initialize();
}
void SwapAreaManager::initialize() {
    totalSwapPages = SWAP_SECTOR_COUNT / 8+1;
    usedSwapPages = 0;
    freeSwapPages = totalSwapPages;
    for (int i = 0; i < totalSwapPages; i++) {
        swapPages[i] = false;
    }
}
int SwapAreaManager::pagenum_to_lba(uint32 pagenum) {
    return SWAP_START_SECTOR + (pagenum-1) * 8;
}
int SwapAreaManager::allocateSwapPage() {
    swapLock.lock();
    for (int i = 1; i < totalSwapPages; i++) {
        if (!swapPages[i]) {
            swapPages[i] = true;
            usedSwapPages++;
            freeSwapPages--;
            swapLock.unlock();
            return i;
        }
    }
    swapLock.unlock();
    return -1;
}
void SwapAreaManager::swapOut(uint32 vaddr,uint32 pagenum) {
    uint32 lba_start = pagenum_to_lba(pagenum);
    uint32 lba_end = lba_start + 8;
    uint32 lba = lba_start;
    while (lba < lba_end) {
        diskIO.writeSector(lba, (void*)vaddr);
        lba++;
        vaddr += 512;
    }
}
void SwapAreaManager::swapIn(uint32 vaddr,uint32 pagenum) {
    uint32 lba_start = pagenum_to_lba(pagenum);
    uint32 lba_end = lba_start + 8;
    uint32 lba = lba_start;
    while (lba < lba_end) {
        diskIO.readSector(lba, (void*)vaddr);
        lba++;
        vaddr += 512;
    }
    swapLock.lock();
    swapPages[pagenum] = false;
    usedSwapPages--;   //对这俩变量要互斥访问
    freeSwapPages++;
    swapLock.unlock();
}
