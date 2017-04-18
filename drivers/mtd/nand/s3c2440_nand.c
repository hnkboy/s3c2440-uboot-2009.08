#include <common.h>

#if 0
#define DEBUGN printf
#else
#define DEBUGN(x, args ...) {}
#endif

#include <nand.h>
#include <s3c2410.h>
#include <asm/io.h>

#define __REGb(x) (*(volatile unsigned char *)(x))
#define __REGi(x) (*(volatile unsigned int *)(x))

#define    NF_BASE        0x4e000000
#define    NFCONF         __REGi(NF_BASE + 0x0)
#define     NFCONT        __REGi(NF_BASE + 0x4)
#define    NFCMD           __REGb(NF_BASE + 0x8)
#define    NFADDR         __REGb(NF_BASE + 0xc)
#define    NFDATA          __REGb(NF_BASE + 0x10)
#define     NFMECCD0    __REGi(NF_BASE + 0x14)
#define     NFMECCD1    __REGi(NF_BASE + 0x18)
#define     NFSECCD       __REGi(NF_BASE + 0x1C)
#define    NFSTAT           __REGb(NF_BASE + 0x20)
#define    NFSTAT0         __REGi(NF_BASE + 0x24)
#define    NFSTAT1         __REGi(NF_BASE + 0x28)
#define    NFMECC0        __REGi(NF_BASE + 0x2C)
#define    NFMECC1        __REGi(NF_BASE + 0x30)
#define    NFSECC           __REGi(NF_BASE + 0x34)
#define    NFSBLK           __REGi(NF_BASE + 0x38)
#define    NFEBLK           __REGi(NF_BASE + 0x3c)

#define S3C2440_NFCONT_nCE    (1<<1)
#define S3C2440_ADDR_NALE 0x0c
#define S3C2440_ADDR_NCLE 0x08

ulong IO_ADDR_W = NF_BASE;//增加一个全局变量

static int s3c2440_dev_ready(struct mtd_info *mtd)
{
DEBUGN("dev_ready\n");
return (NFSTAT & 0x01);
}


static void s3c2440_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
    struct nand_chip *chip = mtd->priv;
    DEBUGN("hwcontrol(): 0x%02x 0x%02x\n", cmd, ctrl);

    if (ctrl & NAND_CTRL_CHANGE) {
        IO_ADDR_W = NF_BASE;
        if (!(ctrl & NAND_CLE)) //要写的是地址
            {
            IO_ADDR_W |= S3C2440_ADDR_NALE;}
        if (!(ctrl & NAND_ALE)) //要写的是命令
            {
            IO_ADDR_W |= S3C2440_ADDR_NCLE;}

        if (ctrl & NAND_NCE)   
            {NFCONT &= ~S3C2440_NFCONT_nCE; //使能nand flash
            //DEBUGN("NFCONT is 0x%x ",NFCONT);
            //DEBUGN("nand Enable ");
            }
        else
            {NFCONT |= S3C2440_NFCONT_nCE;  //禁止nand flash
            //DEBUGN("nand disable ");
            }
    }

    if (cmd != NAND_CMD_NONE)
        writeb(cmd,(void *)IO_ADDR_W);
}

int board_nand_init(struct nand_chip *nand)
{
     u_int32_t cfg;
     u_int8_t tacls, twrph0, twrph1;
     S3C24X0_CLOCK_POWER * const clk_power = S3C24X0_GetBase_CLOCK_POWER();
    
     DEBUGN("board_nand_init()\n");
 
     clk_power->CLKCON |= (1 << 4);


    DEBUGN("CONFIG_S3C2440\n");
        twrph0 = 4; twrph1 = 2; tacls = 0;
    cfg = (tacls<<12)|(twrph0<<8)|(twrph1<<4);
    NFCONF = cfg;
    //DEBUGN("cfg is %x\n",cfg);
    //DEBUGN("NFCONF is %lx\n",NFCONF);
    cfg = (1<<6)|(1<<4)|(0<<1)|(1<<0);
    NFCONT = cfg;
    //DEBUGN("cfg is %lx\n",cfg);
    //DEBUGN("NFCONT is %x\n",NFCONT);

/* initialize nand_chip data structure */
    nand->IO_ADDR_R = nand->IO_ADDR_W = (void *)0x4e000010;

    /* read_buf and write_buf are default */
    /* read_byte and write_byte are default */

    /* hwcontrol always must be implemented */
    nand->cmd_ctrl = s3c2440_hwcontrol;
    nand->dev_ready = s3c2440_dev_ready;
    return 0;
}
