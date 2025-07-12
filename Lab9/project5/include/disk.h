#ifndef DISK_IO_H
#define DISK_IO_H

#include "os_type.h"
#include "sync.h"
// 磁盘分配常量
#define MBR_SECTOR 0                    // MBR扇区
#define BOOTLOADER_START_SECTOR 1       // Bootloader起始扇区
#define BOOTLOADER_SECTOR_COUNT 5       // Bootloader扇区数
#define KERNEL_START_SECTOR 6           // 内核起始扇区  
#define KERNEL_SECTOR_COUNT 145         // 内核扇区数
#define SWAP_START_SECTOR 151           // 交换区起始扇区
#define SWAP_SECTOR_COUNT 8000        // 交换区扇区数 
#define SWAP_END_SECTOR 8151           // 交换区结束扇区
#define TOTAL_SECTORS 20160             // 磁盘总扇区数

// 扇区大小
#define SECTOR_SIZE 512

// ATA端口定义
#define ATA_DATA_PORT 0x1F0
#define ATA_FEATURES_PORT 0x1F1
#define ATA_SECTOR_COUNT_PORT 0x1F2
#define ATA_LBA_LOW_PORT 0x1F3
#define ATA_LBA_MID_PORT 0x1F4
#define ATA_LBA_HIGH_PORT 0x1F5
#define ATA_DRIVE_HEAD_PORT 0x1F6
#define ATA_STATUS_PORT 0x1F7
#define ATA_COMMAND_PORT 0x1F7

// ATA命令
#define ATA_CMD_READ_SECTORS 0x20
#define ATA_CMD_WRITE_SECTORS 0x30

// ATA状态位
#define ATA_STATUS_BSY 0x80    // 忙碌
#define ATA_STATUS_DRQ 0x08    // 数据请求
#define ATA_STATUS_ERR 0x01    // 错误

class DiskIO
{
public:
    DiskIO();
    
    bool readSector(uint32 lba, void* buffer);
    
    bool writeSector(uint32 lba, const void* buffer);
    
    // 检查LBA是否在交换区范围内
    bool isSwapSector(uint32 lba);
    
    // 获取交换区起始扇区
    uint32 getSwapStartSector();
    
    // 获取交换区扇区数量
    uint32 getSwapSectorCount();
    
};

class SwapAreaManager
{
    int totalSwapPages;
    int usedSwapPages;
    int freeSwapPages;
    bool swapPages[1000];  //管理交换区每一页是否被分配
    SpinLock swapLock;
public:
    SwapAreaManager();
    void initialize();
    //页的换出
    void swapOut(uint32 vaddr,uint32 pagenum);  //vaddr:换到外存的页的起始虚拟地址，pagenum:交换区的编号
    //页的换入
    void swapIn(uint32 vaddr,uint32 pagenum);
    int pagenum_to_lba(uint32 pagenum); //页号转换为LBA编号(起始连续八个)
    int allocateSwapPage(); //分配一个交换区页，返回pagenum
};

#endif 