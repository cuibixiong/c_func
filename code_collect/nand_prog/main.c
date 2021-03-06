#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/mman.h>
#include<string.h>
#include<stdlib.h>
#include<stdio.h>

#include "jz4740.h"
#include "nand.h"

unsigned int GPIO_BASE;
unsigned int EMC_BASE;

unsigned int addrport;
unsigned int dataport;
unsigned int cmdport;

/* Modify next parameters according to your flash */
int bus = 8; /* bus width */
int row = 3; /* row cycles */
int pagesize = 2048; /* pagesize */
int oobsize = 64; /* oobsize */
//int ppb = 128; /* pages per block */
int ppb = 64;

static unsigned char nand_buf[(2048+64)*128] = {0};       //Max 128 pages!
static unsigned char check_buf[(2048+64)*128] = {0};

static int check_only = 0;

static int nand_check(u8 *buf1, u8 *buf2, u32 len)
{
	u32 i;

	for (i = 0; i < len; i++)
	{
		if (buf1[i] != buf2[i])
		{
			printf("Check error i=%d 0x%02x 0x%02x\n", i, buf1[i], buf2[i]);
			return -1;
		}
	}
	return 0;
}

/* program a file to a partition */
static int do_prog_part(char *filename, unsigned int start_block, unsigned int last_block, int ecc_pos)
{
	FILE *fp;
	unsigned int flen, offset, fblk, fpage;
	unsigned int curblk, copyblks, badblks;
	unsigned char *tmpbuf;
	int curblk_done = 1, checkerr = 0;
	int actual_pages = ppb;

	if ((fp = fopen(filename, "r")) == NULL) {
		printf("Can not open source or object file!\n");
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	flen = ftell(fp);

	if (flen % (pagesize + oobsize) != 0) {
		printf("%s: filelength is not fit!\n", filename);
		return -1;
	}

	fblk = flen / (ppb * (pagesize + oobsize)); /* total blocks needed */
	fpage = (flen - fblk * (ppb * (pagesize + oobsize))) / (pagesize + oobsize); /* left pages */
		
	printf("file %s: fblk=%d fpage=%d\n", filename, fblk, fpage);

	offset = 0;
	curblk = start_block;

	copyblks = badblks = 0;

	while (curblk <= last_block) {
		int p;

		if (curblk_done) {
			/* Read block data from file */
			if ((copyblks == fblk) && (fpage > 0)) {
				memset(nand_buf, 0xff, ppb * (pagesize + oobsize));
				fseek(fp, offset, SEEK_SET);
				fread(nand_buf, 1, fpage * (pagesize + oobsize), fp);
				actual_pages = fpage;
			}
			else {
				fseek(fp, offset, SEEK_SET);
				fread(nand_buf, 1, ppb * (pagesize + oobsize), fp);
			}
			curblk_done = 0;
		}

		/* check block is bad or not */
		if (nand_check_badblock(curblk)) {
			printf("Bad block %d, skipped\n", curblk);
			curblk++;
			badblks++;
			continue;
		}

		if (!check_only) {
			/* program a good block */
			tmpbuf = (unsigned char *)nand_buf;
			for (p = 0; p < actual_pages; p++) {
				nand_program_page(tmpbuf, tmpbuf + pagesize, curblk * ppb + p, ecc_pos);
				tmpbuf += pagesize + oobsize;
			}
		}

		/* read block data from nand for check */
		tmpbuf = (unsigned char *)check_buf;
		for (p = 0; p < actual_pages; p++) {
			nand_read_page(tmpbuf, tmpbuf + pagesize, curblk * ppb + p, ecc_pos);
			if (check_only) { /* restore 0xff of ecc values in yaffs2 image */
				int j;
				unsigned char *tmp = (unsigned char *) (tmpbuf + pagesize + ecc_pos);
				for (j = 0; j < 36; j++)
					tmp[j] = 0xff;	
			}
			tmpbuf += pagesize + oobsize;
		}

//		if (check_only)
//			printf("Check block %d (seq=%d): ", curblk, copyblks);
		if (nand_check(nand_buf, check_buf, actual_pages * (pagesize + oobsize))) {
			checkerr++;
			if (!check_only) {
				printf("Block %d check error, mark it bad\n", curblk);
				/* Mark this block bad */
				nand_erase_block(curblk);
				nand_mark_badblock(curblk);
				curblk++;
				badblks++;
				continue;
			}
//			else
//				printf("Bad data\n");
		}
//		else {
//			printf("Good data\n");
//		}

		curblk_done = 1;
		offset += ppb * (pagesize + oobsize); 
		curblk++;
		copyblks++;

		if (((copyblks == fblk) && (fpage == 0)) ||
		    ((copyblks > fblk) && (fpage > 0))) {
			if (check_only) {
				printf("check file %s totol_blocks=%d error_blocks=%d\n", filename, copyblks, checkerr);
			}
			else {
				printf("Program file %s finished\n", filename);
			}
			break;
		}
	}

	if (copyblks < fblk)
		printf("Program file %s not finished: copied %d block, required %d blocks\n", filename, copyblks, fblk);

	fclose(fp);

	return 0;
}

static int init_hardware(void)
{
	int fd;
	void *emcbase, *gpiobase, *dportbase, *cportbase, *aportbase;

	fd = open("/dev/mem", O_RDWR);
	if(fd <= 0){
		fprintf(stderr, "ERR: can't open /dev/mem\n");
		return -1;
	}

	emcbase = (void *)mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x13010000);
	if(emcbase == MAP_FAILED) {
		printf("Can not map EMC_BASE ioport!\n");
		return -1;
	}

	gpiobase = (void *)mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x10010000);
	if(gpiobase == MAP_FAILED) {
		printf("Can not map GPIO_BASE ioport!\n");
		return -1;
	}

	dportbase = (void *)mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x18000000);
	if(dportbase == MAP_FAILED) {
		printf("Can not map DATAPORT_BASE ioport!\n");
		return -1;
	}

	cportbase = (void *)mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x18008000);
	if(cportbase == MAP_FAILED) {
		printf("Can not map CMDPORT_BASE ioport!\n");
		return -1;
	}

	aportbase = (void *)mmap(0, 0x1000, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0x18010000);
	if(aportbase == MAP_FAILED) {
		printf("Can not map ADDRPORT_BASE ioport!\n");
		return -1;
	}

	close(fd);

	nand_init_4740((unsigned int)gpiobase, (unsigned int)emcbase, (unsigned int)aportbase, (unsigned int)dportbase, (unsigned int)cportbase);

	return 0;
}
#if 0
int main(int argc, char *argv[])
{
	int start_block, last_block, ecc_pos;

	if (argc > 1)
		check_only = 1;

	init_hardware();

	printf("nand query id=0x%x\n", nand_query_4740());

	/* start programming ... 
	 * add more partitions as required.
	 */
#if 0
	/* part 1: block */
	start_block = 1024; /* 256MB */
	last_block = 2047; /* 512MB */
	ecc_pos = 28; /* ECC offset to oob area */
	do_prog_part("/home/jlwei/test.yaffs2", start_block, last_block, ecc_pos);
#else
/* 64 pages per block */
	start_block = 512; /* 64MB */
	last_block = 1024; /* 128MB */
	ecc_pos = 28; /* ECC offset to oob area */
	do_prog_part("/mnt/test.yaffs2", start_block, last_block, ecc_pos);
#endif

#if 0
	/* part 2: block */
	start_block = 2048; /* 512MB */
	last_block = 4095;  /* 1GB */
	ecc_pos = 28; /* ECC offset to oob area */
	do_prog_part("/home/jlwei/root.yaffs2", start_block, last_block, ecc_pos);
#endif
	return 0;
}
#else

int main(int argc, char *argv[])
{
	int i,curblk, p, ecc_pos;
	unsigned char tmpbuf[4096+128] = {0};

	init_hardware();

	bus = 8;
	row = 3;
	pagesize = 4096;
	oobsize = 128;
	ppb = 128;

	printf("nand query id=0x%x\n", nand_query_4740());

	/* start programming ... 
	 * add more partitions as required.
	 */

	ecc_pos = 6; /* ECC offset to oob area */
	curblk = 0; /* 64MB */
	p = 0;        /* page 0 */

	memset(tmpbuf,0x55,4096);

	for (i=1103;i<1109;i++) {
		nand_erase_block(i);
		nand_program_page(tmpbuf, tmpbuf + pagesize, i*ppb+127, ecc_pos);
		memset(tmpbuf,0x0,4096+128);
		nand_read_page(tmpbuf, tmpbuf + pagesize, i*ppb+127, ecc_pos);
		int h;
		for (h=0;h<16;h++) {
			printf("0x%x ",tmpbuf[4090+h]);
		}
		printf("i=%d\n",i);
	}
	exit(1);


	nand_program_page(tmpbuf, tmpbuf + pagesize, curblk * ppb + p, ecc_pos);
	nand_erase_block(curblk);

	printf("orig buf=%d %d %d %d\n",tmpbuf[0],tmpbuf[7],tmpbuf[17],tmpbuf[27]);

	tmpbuf[0]=5;
	tmpbuf[7]=1;
	tmpbuf[17]=1;
	tmpbuf[27]=1;
	printf("bad buf=%d %d %d %d\n",tmpbuf[0],tmpbuf[7],tmpbuf[17],tmpbuf[27]);
	nand_program_bad_page(tmpbuf, tmpbuf + pagesize, curblk * ppb + p, ecc_pos);

	nand_read_page(tmpbuf, tmpbuf + pagesize, curblk * ppb + p, ecc_pos);
	printf("correct buf=%d %d %d %d\n",tmpbuf[0],tmpbuf[7],tmpbuf[17],tmpbuf[27]);

	return 0;
}

#endif
