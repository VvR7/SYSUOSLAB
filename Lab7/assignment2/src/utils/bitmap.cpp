#include "bitmap.h"
#include "stdlib.h"
#include "stdio.h"

BitMap::BitMap()
{
}

void BitMap::initialize(char *bitmap, const int length, enum AllocateType allocateType)
{
    this->bitmap = bitmap;
    this->length = length;
    this->allocateType = allocateType;

    int bytes = ceil(length, 8);

    for (int i = 0; i < bytes; ++i)
    {
        bitmap[i] = 0;
    }
}

bool BitMap::get(const int index) const
{
    int pos = index / 8;
    int offset = index % 8;

    return (bitmap[pos] & (1 << offset));
}

void BitMap::set(const int index, const bool status)
{
    int pos = index / 8;
    int offset = index % 8;

    // 清0
    bitmap[pos] = bitmap[pos] & (~(1 << offset));

    // 置1
    if (status)
    {
        bitmap[pos] = bitmap[pos] | (1 << offset);
    }
}

int BitMap::allocate(const int count)
{
    if (count == 0)
        return -1;
    int index=-1;
    switch (allocateType)
    {
    case FIRST_FIT:
        index=firstFit(count);
        break;
    case BEST_FIT:
        index=bestFit(count);
        break;
    case WORST_FIT:
        index=worstFit(count);
        break;
    }
    if (index!=-1) {
        for (int i=index;i<index+count;i++) {
            set(i,true);
        }
    }
    if (index!=-1) printf("Allocate count: %d, page %d to %d\n",count,index,index+count-1);
    return index;
}

void BitMap::release(const int index, const int count)
{
    for (int i = 0; i < count; ++i)
    {
        set(index + i, false);
    }
    printf("Release count: %d, page %d to %d\n",count,index,index+count-1);
}

char *BitMap::getBitmap()
{
    return (char *)bitmap;
}

int BitMap::size() const
{
    return length;
}
int BitMap::firstFit(const int count)
{
    int cnt=0;
    for (int i=0;i<length;i++) {
        if (get(i)) {
            cnt=0;
        } else {
            cnt++;
            if (cnt==count) {
                return i-count+1;
            }
        }
    }
    return -1;
}
int BitMap::bestFit(const int count)
{
    int min=1e9;
    int beststart=-1;
    int cnt=0;
    for (int i=0;i<length;i++) {
        if (get(i)) {
            if (cnt>=count) {
                if (cnt-count<min) {
                    min=cnt-count;
                    beststart=i-cnt;
                }
            }
            cnt=0;
        } else {
            cnt++;
        }
    }
    if (cnt>=count) {
        if (cnt-count<min) {
            min=cnt-count;
            beststart=length-cnt;
        }
    }
    return beststart;
}
int BitMap::worstFit(const int count)
{
    int max=0;
    int worststart=-1;
    int cnt=0;
    for (int i=0;i<length;i++) {
        if (get(i)) {
            if (cnt>=count) {
                if (cnt-count>max) {
                    max=cnt-count;
                    worststart=i-cnt;
                }
            }
            cnt=0;
        } else {
            cnt++;
        }
    }
    if (cnt>=count) {
        if (cnt-count>max) {
            max=cnt-count;
            worststart=length-cnt;
        }
    }
    return worststart;
}