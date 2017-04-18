#include <config.h> 

#define NF_BASE   0x4E000000  //Nand Flash配置寄存器基地址

#define __REGb(x) (*(volatile unsigned char *)(x))
#define __REGi(x) (*(volatile unsigned int  *)(x))

#define NFCONF __REGi(NF_BASE + 0x0 )  //通过偏移量还是得到配置寄存器基地址
#define NFCONT __REGi(NF_BASE + 0x4 )  //通过偏移量得到控制寄存器基地址
#define NFCMD  __REGb(NF_BASE + 0x8 )  //通过偏移量得到指令寄存器基地址
#define NFADDR __REGb(NF_BASE + 0xC )  //通过偏移量得到地址寄存器基地址
#define NFDATA __REGb(NF_BASE + 0x10)  //通过偏移量得到数据寄存器基地址
#define NFSTAT __REGb(NF_BASE + 0x20)  //通过偏移量得到状态寄存器基地址

#define NAND_CHIP_ENABLE  (NFCONT &= ~(1<<1))  //Nand片选使能
#define NAND_CHIP_DISABLE (NFCONT |= (1<<1))   //取消Nand片选
#define NAND_CLEAR_RB     (NFSTAT |= (1<<2))
#define NAND_DETECT_RB    { while(! (NFSTAT&(1<<2)) );}

#define NAND_SECTOR_SIZE 512
#define NAND_BLOCK_MASK (NAND_SECTOR_SIZE - 1)

/* low level nand read function */
int nand_read_ll(unsigned char *buf, unsigned long start_addr, int size)
{
    int i, j;

    if ((start_addr & NAND_BLOCK_MASK) || (size & NAND_BLOCK_MASK)) 
    {
        return -1; //地址或长度不对齐
    }

    NAND_CHIP_ENABLE; //选中Nand片选

    for(i=start_addr; i < (start_addr + size);) 
    {
        //发出READ0指令
        NAND_CLEAR_RB;
        NFCMD = 0;

        //对Nand进行寻址
        NFADDR = i & 0xFF;
        NFADDR = (i >> 9) & 0xFF;
        NFADDR = (i >> 17) & 0xFF;
        NFADDR = (i >> 25) & 0xFF;

        NAND_DETECT_RB;

        for(j=0; j < NAND_SECTOR_SIZE; j++, i++) 
        {
            *buf = (NFDATA & 0xFF);
            buf++;
        }
    }

    NAND_CHIP_DISABLE; //取消片选信号

    return 0;
}
