#include "os_type.h"

template<typename T>
void swap(T &x, T &y) {
    T z = x;
    x = y;
    y = z;
}


void itos(char *numStr, uint32 num, uint32 mod) {
    // 只能转换2~26进制的整数
    if (mod < 2 || mod > 26 || num < 0) {
        return;
    }

    uint32 length, temp;

    // 进制转换
    length = 0;
    while(num) {
        temp = num % mod;
        num /= mod;
        numStr[length] = temp > 9 ? temp - 10 + 'A' : temp + '0';
        ++length;
    }

    // 特别处理num=0的情况
    if(!length) {
        numStr[0] = '0';
        ++length;
    }

    // 将字符串倒转，使得numStr[0]保存的是num的高位数字
    for(int i = 0, j = length - 1; i < j; ++i, --j) {
        swap(numStr[i], numStr[j]);
    }
    
    numStr[length] = '\0';
}
void ftos(char *numStr, double num,int precision)
{
    int i=0;
    int intpart=(int)num;  //整数部分
    double frac=num-intpart;  //小数部分

    char intBuf[33];
    int int_idx=0;
    if (intpart==0) {
        intBuf[int_idx++]='0';
    } else {
        while(intpart>0) {
            intBuf[int_idx++]='0'+(intpart%10);
            intpart/=10;
        }
    }
    for (int j=int_idx-1;j>=0;j--)
      numStr[i++]=intBuf[j];
    if (precision>0) {
        numStr[i++]='.';
        int x=1;
        for (int j=0;j<precision;j++) {
            x*=10;
        }
        frac=frac*x+0.5; //四舍五入
        int fracint=(int)frac;
        int fracidx=0;
        char fracBuf[33];
        for (int j=0;j<precision;j++) {
            fracBuf[precision-1-j]='0'+(fracint%10);
            fracint/=10;
        }
        for (int j=0;j<precision;j++)
        {
            numStr[i++]=fracBuf[j];
        }
    }
    numStr[i]='\0';
}
void double_to_e(char *str, double num,int precision) {
    int i = 0;
    if (num < 0.0) {
        str[i++] = '-';
        num = -num;
    }
    if (num == 0.0) {
        str[i++] = '0';
        str[i++] = '.';
        for (int j = 0; j < precision; j++) {
            str[i++] = '0';
        }
        str[i++] = 'e';
        str[i++] = '+';
        str[i++] = '0';
        str[i++] = '0';
        str[i]   = '\0';
        return;
    }
    //归一化，统计指数
    int exp = 0;
    while (num >= 10.0) { num /= 10.0; exp++; }
    while (num <  1.0) { num *= 10.0; exp--; }

    //四舍五入
    int mult = 1;
    for (int j = 0; j < precision; j++) {
        mult *= 10;
    }
    int total = (int)(num * mult + 0.5);

    // 5) 处理四舍五入导致的进位溢出
    if (total >= mult * 10) {
        total /= 10;
        exp++;
    }

    //拆整数位和小数位
    int intPart  = total / mult;  // 1～9
    int fracPart = total % mult;  // 0 … mult-1

    //输出小数点前的部分
    str[i++] = '0' + intPart;
    str[i++] = '.';

    //输出小数部分
    {
        char buf[10];
        for (int j = precision - 1; j >= 0; j--) {
            buf[j] = '0' + (fracPart % 10);
            fracPart /= 10;
        }
        for (int j = 0; j < precision; j++) {
            str[i++] = buf[j];
        }
    }

    //输出指数标志和符号
    str[i++] = 'e';
    if (exp >= 0) {
        str[i++] = '+';
    } else {
        str[i++] = '-';
        exp = -exp;
    }

    //输出指数，至少两位（不足补零），可支持更多位
    {
        char buf[10];
        int idx = 0;
        do {
            buf[idx++] = '0' + (exp % 10);
            exp /= 10;
        } while (exp > 0);
        // 至少两位
        while (idx < 2) {
            buf[idx++] = '0';
        }
        // 反向写入
        for (int j = idx - 1; j >= 0; j--) {
            str[i++] = buf[j];
        }
    }
    str[i] = '\0';
}
