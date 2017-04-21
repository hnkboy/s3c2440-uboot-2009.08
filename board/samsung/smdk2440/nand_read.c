/*
 * nand_read.c: Simple NAND read functions for booting from NAND
 *
 * This is used by cpu/arm920/start.S assembler code,
 * and the board-specific linker script must make sure this
 * file is linked within the first 4kB of NAND flash.
 *
 * Taken from GPLv2 licensed vivi bootloader,
 * Copyright (C) 2002 MIZI Research, Inc.
 *
 * Author: Hwang, Chideok <hwang@mizi.com>
 * Date  : $Date: 2004/02/04 10:37:37 $
 *
 * u-boot integration and bad-block skipping (C) 2006 by OpenMoko, Inc.
 * Author: Harald Welte <laforge@openmoko.org>
 */

#include <common.h>
#include <linux/mtd/nand.h>

#define __REGb(x)	(*(volatile unsigned char *)(x))
#define __REGw(x)	(*(volatile unsigned short *)(x))
#define __REGi(x)	(*(volatile unsigned int *)(x))

#define NF_BASE		0x4e000000


#define NFCONF		__REGi(NF_BASE + 0x0)
#define NFCONT		__REGi(NF_BASE + 0x4)
#define NFCMD		__REGb(NF_BASE + 0x8)
#define NFADDR		__REGb(NF_BASE + 0xc)
#define NFDATA		__REGb(NF_BASE + 0x10)
#define NFSTAT		__REGb(NF_BASE + 0x20)

#define NFSTAT_BUSY	1
#define nand_select()	(NFCONT &= ~(1 << 1))
#define nand_deselect()	(NFCONT |= (1 << 1))
#define nand_clear_RnB()	(NFSTAT |= (1 << 2))

/*NFCONF…Ë÷√ ±–Ú*/
#define TACLS	1
#define TWRPH0	2
#define TWRPH1	0


struct boot_nand_t {
	int page_size;
	int block_size;
	int bad_block_offset;
	//	unsigned long size;
};


/* -----------µ»¥˝√¸¡Ó÷¥––ÕÍ------------*/
static inline void nand_wait(void)
{
	int i;
	
	while (!(NFSTAT & NFSTAT_BUSY))
		for (i=0; i<10; i++);
}


/* -----------ºÏ≤ÈªµøÈ£¨√ø“ªøÈµƒµ⁄1~2“≥µƒ2048µÿ÷∑------------*/
static int is_bad_block(struct boot_nand_t * nand, unsigned long addr)
{
	unsigned char data;
	unsigned long page_num;
	
	nand_clear_RnB();
	if (nand->page_size == 2048) {
		page_num = addr >> 11; /* addr / 2048 */
		NFCMD = NAND_CMD_READ0;
		NFADDR = nand->bad_block_offset & 0xff;
		NFADDR = (nand->bad_block_offset >> 8) & 0xff;
		NFADDR = page_num & 0xff;
		NFADDR = (page_num >> 8) & 0xff;
		NFADDR = (page_num >> 16) & 0xff;
		NFCMD = NAND_CMD_READSTART;
	} else {
		return -1;
	}
	nand_wait();
	data = (NFDATA & 0xff);
	if (data != 0xff)
		return 1;//==error
	
	return 0;
}


/* -----------nand∂¡“ª“≥------------*/
static int nand_read_page(struct boot_nand_t * nand, unsigned char *buf, unsigned long addr)
{
	unsigned char *ptr8 = buf;
	unsigned int i, page_num;
	
	nand_clear_RnB();
	
	NFCMD = NAND_CMD_READ0;
	
	if (nand->page_size == 2048) {
		page_num = addr >> 11; /* addr / 2048 */
		/* Write Address */
		NFADDR = 0;
		NFADDR = 0;
		NFADDR = page_num & 0xff;
		NFADDR = (page_num >> 8) & 0xff;
		NFADDR = (page_num >> 16) & 0xff;
		NFCMD = NAND_CMD_READSTART;
	} else {
		return -1;
	}
	nand_wait();
	
	for (i = 0; i < (nand->page_size); i++) {
		*ptr8 = NFDATA;
		ptr8++;
	}
	
	return nand->page_size;
}


/* -----------nand∂¡ID------------*/
static unsigned short nand_read_id()
{
	unsigned short res = 0;
	
	nand_clear_RnB();
	NFCMD = NAND_CMD_READID;
	NFADDR = 0;
	res = NFDATA;
	res = (res << 8) | NFDATA;
	return res;
}


/* -----------low level nand read function ------------*/
int nand_read_ll(unsigned char *buf, unsigned long start_addr, int size)
{
	int i, j;
	unsigned short nand_id;
	struct boot_nand_t nand;
	
	/* chip Enable */
	NFCONF = (TACLS<<12)|(TWRPH0<<8)|(TWRPH1<<4)|(0<<0);
	NFCONT = (1<<4)|(1<<1)|(1<<0);	// πƒ‹ECC,  πƒ‹flash, »°œ˚∆¨—°
	
	nand_select();
	nand_clear_RnB();
	NFCMD = NAND_CMD_RESET;
	nand_wait();
	
	nand_id = nand_read_id();
	
	if (nand_id == 0xec76 ||		/* Samsung K91208 */
		nand_id == 0xad76 ) {	/*Hynix HY27US08121A*/
		nand.page_size = 512;
		nand.block_size = 16 * 1024;
		nand.bad_block_offset = 5;
		//	nand.size = 0x4000000;
	} else if (nand_id == 0xecf1 ||	/* Samsung K9F1G08U0B */
			   nand_id == 0xecda ||	/* Samsung K9F2G08U0B */
			   nand_id == 0xecd3 ){	/* Samsung K9K8G08 */
		
		nand.page_size = 2048;
		nand.block_size = 128 * 1024;
		nand.bad_block_offset = nand.page_size;
		//	nand.size = 0x8000000;
	} else {
		return -1; // hang
	}
	
	if ( (start_addr & (nand.block_size-1)) || (size &(nand.block_size-1)) )
		return -1;	/* invalid alignment */
	
	for (i=start_addr; i < (start_addr + size);) {
#if defined (CONFIG_S3C2440_NAND_SKIP_BAD)
		if (i & (nand.block_size-1)== 0) {
			if (is_bad_block(&nand, i) ||
				is_bad_block(&nand, i + nand.page_size)) {
				/* Bad block */
				i += nand.block_size;
				size += nand.block_size;
				continue;
			}
		}
#endif
		j = nand_read_page(&nand, buf, i);
		i += j;
		buf += j;
	}
	
	/* chip Disable */
	nand_deselect();
	
	return 0;
}
