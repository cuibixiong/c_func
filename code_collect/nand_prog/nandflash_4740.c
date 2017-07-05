/*
 * nandflash_4740.c
 */

#include <stdio.h>
#include "nand.h"
#include "jz4740.h"
#include <sys/time.h>

#define __nand_enable()		(REG_EMC_NFCSR |= EMC_NFCSR_NFE1 | EMC_NFCSR_NFCE1)
#define __nand_disable()	(REG_EMC_NFCSR &= ~(EMC_NFCSR_NFCE1))
#define __nand_ecc_rs_encoding()	(REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST | EMC_NFECR_RS | EMC_NFECR_RS_ENCODING)
#define __nand_ecc_rs_decoding()	(REG_EMC_NFECR = EMC_NFECR_ECCE | EMC_NFECR_ERST | EMC_NFECR_RS | EMC_NFECR_RS_DECODING)
#define __nand_ecc_disable()	(REG_EMC_NFECR &= ~EMC_NFECR_ECCE)
#define __nand_ecc_encode_sync() while (!(REG_EMC_NFINTS & EMC_NFINTS_ENCF))
#define __nand_ecc_decode_sync() while (!(REG_EMC_NFINTS & EMC_NFINTS_DECF))

#define __nand_ready()		((REG_GPIO_PXPIN(2) & 0x40000000) ? 1 : 0)
#define __nand_ecc()		(REG_EMC_NFECC & 0x00ffffff)
#define __nand_cmd(n)		(REG8(cmdport) = (n))
#define __nand_addr(n)		(REG8(addrport) = (n))
#define __nand_data8()		REG8(dataport)
#define __nand_data16()		REG16(dataport)

#define CMD_READA	0x00
#define CMD_READB	0x01
#define CMD_READC	0x50
#define CMD_ERASE_SETUP	0x60
#define CMD_ERASE	0xD0
#define CMD_READ_STATUS 0x70
#define CMD_CONFIRM	0x30
#define CMD_SEQIN	0x80
#define CMD_PGPROG	0x10
#define CMD_READID	0x90

#define ECC_BLOCK	512
#define PAR_SIZE        9

//static u8 data_buf[2048] = {0};
//static u8 oob_buf[128] = {0};

static inline void __nand_sync(void)
{
	unsigned int timeout = 100;
	while ((REG_GPIO_PXPIN(2) & 0x40000000) && timeout--);
	while (!(REG_GPIO_PXPIN(2) & 0x40000000));
}

static int read_oob(void *buf, unsigned int size, unsigned int pg);
static int nand_data_write8(char *buf, int count);
static int nand_data_write16(char *buf, int count);
static int nand_data_read8(char *buf, int count);
static int nand_data_read16(char *buf, int count);

static int (*write_proc)(char *, int) = NULL;
static int (*read_proc)(char *, int) = NULL;

extern void dumpbuf(u8 *p, int count);


unsigned int nand_query_4740(void)
{
	u8 vid, did;

	__nand_cmd(CMD_READID);
	__nand_addr(0);

	vid = __nand_data8();
	did = __nand_data8();

	return (vid << 8) | did;
}

int nand_init_4740(unsigned int gbase, unsigned int ebase, unsigned int aport, unsigned int dport, unsigned int cport)
{
	bus = 8;
	row = 3;
	pagesize = 2048;
	oobsize = 64;
	ppb = 128;
//	ppb = 64;

	GPIO_BASE = gbase;
	EMC_BASE = ebase;
	addrport = aport;
	dataport = dport;
	cmdport = cport;

	__gpio_as_nand();

//	REG_EMC_SMCR1 = 0x09221200;
	REG_EMC_SMCR1 = 0x0fffff00;

//	__nand_enable();

	if (bus == 8) {
		write_proc = nand_data_write8;
		read_proc = nand_data_read8;
	} else {
		write_proc = nand_data_write16;
		read_proc = nand_data_read16;
	}
	return 0;
}

int nand_fini_4740(void)
{
	__nand_disable();
	return 0;
}

int nand_check_badblock(unsigned int blk)
{
	unsigned int pg;
	u8 oob_buf[128];

	pg = (blk + 1) * ppb - 1; /* the last page of the block */

	read_oob(oob_buf, oobsize, pg);
	if ((oob_buf[0] != 0xff) ||
	    (oob_buf[1] != 0xff))
		return 1;

	return 0;
}

static u8 badbuf[4096 + 128] = {0};

int nand_mark_badblock(unsigned int blk)
{
	int i;
	unsigned int pageaddr = (blk + 1) * ppb - 1; /* the last page of the block */

	__nand_enable();

	for (i = 0; i < pagesize + oobsize; i++)
		badbuf[i] = 0;

	if (pagesize != 2048)
		__nand_cmd(CMD_READA);

	__nand_cmd(CMD_SEQIN);

	/* write out col addr */
	__nand_addr(0);
	if (pagesize == 2048)
		__nand_addr(0);

	/* write out row addr */
	for (i = 0; i < row; i++) {
		__nand_addr(pageaddr & 0xff);
		pageaddr >>= 8;
	}

	/* write out data */
	write_proc((char *)badbuf, pagesize + oobsize);

	/* send program confirm command */
	__nand_cmd(CMD_PGPROG);
	__nand_sync();

	__nand_cmd(CMD_READ_STATUS);

	if (__nand_data8() & 0x01) { /* page program error */
		__nand_disable();
		return 1;
	}
	else {
		__nand_disable();
		return 0;
	}
}

int nand_erase_block(unsigned int blk)
{
	int i;
	unsigned int rowaddr;

	__nand_enable();

	rowaddr = blk * ppb;

	__nand_cmd(CMD_ERASE_SETUP);

	for (i = 0; i < row; i++) {
		__nand_addr(rowaddr & 0xff);
		rowaddr >>= 8;
	}
	__nand_cmd(CMD_ERASE);
	__nand_sync();

	__nand_cmd(CMD_READ_STATUS);

	if (__nand_data8() & 0x01) {
		__nand_disable();
		return 1; /* failed */
	}
	else {
		__nand_disable();
		return 0; /* succeeded */
	}
}

static int read_oob(void *buf, unsigned int size, unsigned int pg)
{
	unsigned int i, coladdr, rowaddr;

	__nand_enable();

	if (pagesize == 512)
		coladdr = 0;
	else
		coladdr = pagesize;

	if (pagesize == 512)
		/* Send READOOB command */
		__nand_cmd(CMD_READC);
	else
		/* Send READ0 command */
		__nand_cmd(CMD_READA);

	/* Send column address */
	__nand_addr(coladdr & 0xff);
	if (pagesize != 512)
		__nand_addr(coladdr >> 8);

	/* Send page address */
	rowaddr = pg;
	for (i = 0; i < row; i++) {
		__nand_addr(rowaddr & 0xff);
		rowaddr >>= 8;
	}

	/* Send READSTART command for 2048 ps NAND */
	if (pagesize != 512)
		__nand_cmd(CMD_CONFIRM);

	/* Wait for device ready */
	__nand_sync();

	/* Read oob data */
	read_proc(buf, size);

	__nand_disable();

	return 0;
}

static void rs_correct(unsigned char *dat, int idx, int mask)
{
	int i, j;
	unsigned short d, d1, dm;

	printf("error idx:%x mask:%x\n",idx,mask);

	i = (idx * 9) >> 3;
	j = (idx * 9) & 0x7;

	i = (j == 0) ? (i - 1) : i;
	j = (j == 0) ? 7 : (j - 1);

	if (i > 512) return;

	if (i == 512)
		d = dat[i - 1];
	else
		d = (dat[i] << 8) | dat[i - 1];

	d1 = (d >> j) & 0x1ff;
	d1 ^= mask;

	dm = ~(0x1ff << j);
	d = (d & dm) | (d1 << j);

	dat[i - 1] = d & 0xff;
	if (i < 512)
		dat[i] = (d >> 8) & 0xff;
}

int nand_read_page(unsigned char *databuf, unsigned char *oobbuf, unsigned int pageaddr, int ecc_pos)
{
	unsigned int i, j;
	unsigned int rowaddr, ecccnt;
	u8 *tmpbuf;

	__nand_enable();

	ecccnt = pagesize / ECC_BLOCK;

	/* read oob first */
	read_oob(oobbuf, oobsize, pageaddr);
#if 0
	for (i=50;i<127;i++)
		printf("%x ",oobbuf[i]);
	printf("nand_read_page1\n");
#endif
	__nand_cmd(CMD_READA);

	__nand_addr(0);
	if (pagesize != 512)
		__nand_addr(0);

	rowaddr = pageaddr;
	for (i = 0; i < row; i++) {
		__nand_addr(rowaddr & 0xff);
		rowaddr >>= 8;
	}

	if (pagesize != 512)
		__nand_cmd(CMD_CONFIRM);

	__nand_sync();
		
	tmpbuf = (u8 *)databuf;

	for (i = 0; i < ecccnt; i++) {
		volatile u8 *paraddr = (volatile u8 *)EMC_NFPAR0;
		unsigned int stat;

		/* Read data */
		REG_EMC_NFINTS = 0x0;
		__nand_ecc_rs_decoding();

		read_proc((char *)tmpbuf, ECC_BLOCK);
#if 0
		printf("i=%d--\n",i);
		if (i==7)
			for (i=0;i<16;i++)
				printf("%x ",tmpbuf[i]);
#endif
		/* Set PAR values */
		for (j = 0; j < PAR_SIZE; j++) {
			*paraddr++ = oobbuf[ecc_pos + i*PAR_SIZE + j];
		}

		/* Set PRDY */
		REG_EMC_NFECR |= EMC_NFECR_PRDY;

		/* Wait for completion */
		__nand_ecc_decode_sync();
		__nand_ecc_disable();

		/* Check decoding */
		stat = REG_EMC_NFINTS;
		
		if (stat & EMC_NFINTS_ERR) {
			//printf("Error occurred\n");
			if (stat & EMC_NFINTS_UNCOR) {
				printf("Uncorrectable ECC error occurred\n");
			}
			else {
				unsigned int errcnt = (stat & EMC_NFINTS_ERRCNT_MASK) >> EMC_NFINTS_ERRCNT_BIT;
				switch (errcnt) {
				case 4: printf("4 errors\n");
					rs_correct(tmpbuf, (REG_EMC_NFERR3 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR3 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				case 3: printf("3 errors\n");
					rs_correct(tmpbuf, (REG_EMC_NFERR2 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR2 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				case 2: printf("2 errors\n");
					rs_correct(tmpbuf, (REG_EMC_NFERR1 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR1 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				case 1: printf("1 error\n");
					rs_correct(tmpbuf, (REG_EMC_NFERR0 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR0 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
					break;
				default:  printf("no error\n");
					break;
				}
			}
		}
		/* increment pointer */
		tmpbuf += ECC_BLOCK;
	}

	__nand_disable();

	return 0;
}

int nand_program_page(unsigned char *databuf, unsigned char *oobbuf, unsigned int pageaddr, int ecc_pos)
{
	int i, j, ecccnt;
	u8 *tmpbuf;
	u8 ecc_buf[128];

	__nand_enable();

	tmpbuf = (u8 *)databuf;

	ecccnt = pagesize / ECC_BLOCK;

	if (pagesize == 512)
		__nand_cmd(CMD_READA);

	__nand_cmd(CMD_SEQIN);

	/* write out col addr */
	__nand_addr(0);
	if (pagesize !=512)
		__nand_addr(0);

	/* write out row addr */
	for (i = 0; i < row; i++) {
		__nand_addr(pageaddr & 0xff);
		pageaddr >>= 8;
	}

	/* write out data */
	tmpbuf = databuf;
	for (i = 0; i < ecccnt; i++) {
		volatile u8 *paraddr = (volatile u8 *)EMC_NFPAR0;
		
		REG_EMC_NFINTS = 0x0;
		__nand_ecc_rs_encoding();
		write_proc((char *)tmpbuf, ECC_BLOCK);
		__nand_ecc_encode_sync();
		__nand_ecc_disable();
			
		/* Read PAR values */
		for (j = 0; j < PAR_SIZE; j++) {
			ecc_buf[i * PAR_SIZE + j] = *paraddr++;
		}

		tmpbuf += ECC_BLOCK;
	}

	for (i = 0; i < ecc_pos; i++) {
		oobbuf[i] = 0xff;
	}

	/* pad ecc bytes to oob buffer */
	for (i = 0; i < ecccnt * PAR_SIZE; i++) {
		oobbuf[ecc_pos + i] = ecc_buf[i];
	}

	for (i = 0; i < PAR_SIZE; i++) {
//		printf("ecc%x ",ecc_buf[i]);
	}
//	printf("\n");

	/* write out oob buffer */
	write_proc((char *)oobbuf, oobsize);

	/* send program confirm command */
	__nand_cmd(CMD_PGPROG);
	__nand_sync();

	__nand_cmd(CMD_READ_STATUS);

	if (__nand_data8() & 0x01) { /* page program error */
		__nand_disable();
		return 1;
	}
	else {
		__nand_disable();
		return 0;
	}
}

static int nand_data_write8(char *buf, int count)
{
	int i;
	u8 *p = (u8 *)buf;
	for (i=0;i<count;i++)
		__nand_data8() = *p++;
	return 0;
}

static int nand_data_write16(char *buf, int count)
{
	int i;
	u16 *p = (u16 *)buf;
	for (i=0;i<count/2;i++)
		__nand_data16() = *p++;
	return 0;
}

static int nand_data_read8(char *buf, int count)
{
	int i;
	u8 *p = (u8 *)buf;
	for (i=0;i<count;i++)
		*p++ = __nand_data8();
	return 0;
}

static int nand_data_read16(char *buf, int count)
{
	int i;
	u16 *p = (u16 *)buf;
	for (i=0;i<count/2;i++)
		*p++ = __nand_data16();
	return 0;
}


int nand_program_bad_page(unsigned char *databuf, unsigned char *oobbuf, unsigned int pageaddr, int ecc_pos)
{
	int i, j, ecccnt;
	u8 *tmpbuf;
	u8 ecc_buf[64];

	__nand_enable();

	tmpbuf = (u8 *)databuf;

	ecccnt = pagesize / ECC_BLOCK;

	if (pagesize != 2048)
		__nand_cmd(CMD_READA);

	__nand_cmd(CMD_SEQIN);

	/* write out col addr */
	__nand_addr(0);
	if (pagesize == 2048)
		__nand_addr(0);

	/* write out row addr */
	for (i = 0; i < row; i++) {
		__nand_addr(pageaddr & 0xff);
		pageaddr >>= 8;
	}

	/* write out data */
	tmpbuf = databuf;
	for (i = 0; i < ecccnt; i++) {

		volatile u8 *paraddr = (volatile u8 *)EMC_NFPAR0;
		
		REG_EMC_NFINTS = 0x0;
		__nand_ecc_rs_encoding();
		write_proc((char *)tmpbuf, ECC_BLOCK);
		__nand_ecc_encode_sync();
		__nand_ecc_disable();
			
		/* Read PAR values */
		for (j = 0; j < PAR_SIZE; j++) {
			ecc_buf[i * PAR_SIZE + j] = *paraddr++;
		}

		tmpbuf += ECC_BLOCK;
	}
#if 0
	/* pad ecc bytes to oob buffer */
	for (i = 0; i < ecccnt * PAR_SIZE; i++) {
		oobbuf[ecc_pos + i] = ecc_buf[i];
	}
#endif
	/* write out oob buffer */
	write_proc((char *)oobbuf, oobsize);

	/* send program confirm command */
	__nand_cmd(CMD_PGPROG);
	__nand_sync();

	__nand_cmd(CMD_READ_STATUS);

	if (__nand_data8() & 0x01) { /* page program error */
		__nand_disable();
		return 1;
	}
	else {
		__nand_disable();
		return 0;
	}
}




typedef struct {
	unsigned char colParity;
	unsigned lineParity;
	unsigned lineParityPrime;
} yaffs_ECCOther;


static const unsigned char column_parity_table[] = {
	0x00, 0x55, 0x59, 0x0c, 0x65, 0x30, 0x3c, 0x69,
	0x69, 0x3c, 0x30, 0x65, 0x0c, 0x59, 0x55, 0x00,
	0x95, 0xc0, 0xcc, 0x99, 0xf0, 0xa5, 0xa9, 0xfc,
	0xfc, 0xa9, 0xa5, 0xf0, 0x99, 0xcc, 0xc0, 0x95,
	0x99, 0xcc, 0xc0, 0x95, 0xfc, 0xa9, 0xa5, 0xf0,
	0xf0, 0xa5, 0xa9, 0xfc, 0x95, 0xc0, 0xcc, 0x99,
	0x0c, 0x59, 0x55, 0x00, 0x69, 0x3c, 0x30, 0x65,
	0x65, 0x30, 0x3c, 0x69, 0x00, 0x55, 0x59, 0x0c,
	0xa5, 0xf0, 0xfc, 0xa9, 0xc0, 0x95, 0x99, 0xcc,
	0xcc, 0x99, 0x95, 0xc0, 0xa9, 0xfc, 0xf0, 0xa5,
	0x30, 0x65, 0x69, 0x3c, 0x55, 0x00, 0x0c, 0x59,
	0x59, 0x0c, 0x00, 0x55, 0x3c, 0x69, 0x65, 0x30,
	0x3c, 0x69, 0x65, 0x30, 0x59, 0x0c, 0x00, 0x55,
	0x55, 0x00, 0x0c, 0x59, 0x30, 0x65, 0x69, 0x3c,
	0xa9, 0xfc, 0xf0, 0xa5, 0xcc, 0x99, 0x95, 0xc0,
	0xc0, 0x95, 0x99, 0xcc, 0xa5, 0xf0, 0xfc, 0xa9,
	0xa9, 0xfc, 0xf0, 0xa5, 0xcc, 0x99, 0x95, 0xc0,
	0xc0, 0x95, 0x99, 0xcc, 0xa5, 0xf0, 0xfc, 0xa9,
	0x3c, 0x69, 0x65, 0x30, 0x59, 0x0c, 0x00, 0x55,
	0x55, 0x00, 0x0c, 0x59, 0x30, 0x65, 0x69, 0x3c,
	0x30, 0x65, 0x69, 0x3c, 0x55, 0x00, 0x0c, 0x59,
	0x59, 0x0c, 0x00, 0x55, 0x3c, 0x69, 0x65, 0x30,
	0xa5, 0xf0, 0xfc, 0xa9, 0xc0, 0x95, 0x99, 0xcc,
	0xcc, 0x99, 0x95, 0xc0, 0xa9, 0xfc, 0xf0, 0xa5,
	0x0c, 0x59, 0x55, 0x00, 0x69, 0x3c, 0x30, 0x65,
	0x65, 0x30, 0x3c, 0x69, 0x00, 0x55, 0x59, 0x0c,
	0x99, 0xcc, 0xc0, 0x95, 0xfc, 0xa9, 0xa5, 0xf0,
	0xf0, 0xa5, 0xa9, 0xfc, 0x95, 0xc0, 0xcc, 0x99,
	0x95, 0xc0, 0xcc, 0x99, 0xf0, 0xa5, 0xa9, 0xfc,
	0xfc, 0xa9, 0xa5, 0xf0, 0x99, 0xcc, 0xc0, 0x95,
	0x00, 0x55, 0x59, 0x0c, 0x65, 0x30, 0x3c, 0x69,
	0x69, 0x3c, 0x30, 0x65, 0x0c, 0x59, 0x55, 0x00,
};

/* Count the bits in an unsigned char or a U32 */

static int yaffs_CountBits(unsigned char x)
{
	int r = 0;
	while (x) {
		if (x & 1)
			r++;
		x >>= 1;
	}
	return r;
}

static int yaffs_CountBits32(unsigned x)
{
	int r = 0;
	while (x) {
		if (x & 1)
			r++;
		x >>= 1;
	}
	return r;
}
/*
 * ECCxxxOther does ECC calcs on arbitrary n bytes of data
 */
void yaffs_ECCCalculateOther(const unsigned char *data, unsigned nBytes,
			     yaffs_ECCOther * eccOther)
{
	unsigned int i;

	unsigned char col_parity = 0;
	unsigned line_parity = 0;
	unsigned line_parity_prime = 0;
	unsigned char b;

	for (i = 0; i < nBytes; i++) {
		b = column_parity_table[*data++];
		col_parity ^= b;

		if (b & 0x01)	 {
			/* odd number of bits in the byte */
			line_parity ^= i;
			line_parity_prime ^= ~i;
		}

	}

	eccOther->colParity = (col_parity >> 2) & 0x3f;
	eccOther->lineParity = line_parity;
	eccOther->lineParityPrime = line_parity_prime;
}

int yaffs_ECCCorrectOther(unsigned char *data, unsigned nBytes,
			  yaffs_ECCOther * read_ecc,
			  const yaffs_ECCOther * test_ecc)
{
	unsigned char cDelta;	/* column parity delta */
	unsigned lDelta;	/* line parity delta */
	unsigned lDeltaPrime;	/* line parity delta */
	unsigned bit;

	cDelta = read_ecc->colParity ^ test_ecc->colParity;
	lDelta = read_ecc->lineParity ^ test_ecc->lineParity;
	lDeltaPrime = read_ecc->lineParityPrime ^ test_ecc->lineParityPrime;

	if ((cDelta | lDelta | lDeltaPrime) == 0)
		return 0; /* no error */

	if (lDelta == ~lDeltaPrime && 
	    (((cDelta ^ (cDelta >> 1)) & 0x15) == 0x15))
	{
		/* Single bit (recoverable) error in data */

		bit = 0;

		if (cDelta & 0x20)
			bit |= 0x04;
		if (cDelta & 0x08)
			bit |= 0x02;
		if (cDelta & 0x02)
			bit |= 0x01;

		if(lDelta >= nBytes)
			return -1;
			
		data[lDelta] ^= (1 << bit);

		return 1; /* corrected */
	}

	if ((yaffs_CountBits32(lDelta) + yaffs_CountBits32(lDeltaPrime) +
	     yaffs_CountBits(cDelta)) == 1) {
		/* Reccoverable error in ecc */

		*read_ecc = *test_ecc;
		return 1; /* corrected */
	}

	/* Unrecoverable error */

	return -1;

}


int rs_enc(void)
{
	int j;
	u8 tmpbuf[512];
	u8 ecc_buf[64];
	volatile u8 *paraddr = (volatile u8 *)EMC_NFPAR0;

	for (j=0;j<512;j++){
		tmpbuf[j] = j;
	}
	yaffs_ECCOther  ecc;

	yaffs_ECCCalculateOther(tmpbuf, 16, &ecc);
	yaffs_ECCCorrectOther(tmpbuf, 16,&ecc,&ecc);



	__nand_enable();

	REG_EMC_NFINTS = 0x0;
	__nand_ecc_rs_encoding();

//	write_proc((char *)tmpbuf, 512);
//	memcpy((char *)dataport, tmpbuf, 512);
	for (j=0;j<512;j++){
		*(char *)dataport = tmpbuf[j];
	}

	__nand_ecc_encode_sync();
	__nand_ecc_disable();


	/* Read PAR values */
	for (j = 0; j < 9; j++) {
		ecc_buf[j] = *paraddr++;
		printf("%x ",ecc_buf[j]);
	}
	printf("\n");

#if 1
	tmpbuf[0] ^=0x1;
	tmpbuf[3] ^=0x1;
	tmpbuf[6] ^=0x1;
	tmpbuf[9] ^=0x1;
#endif
	for(j=0;j<10;j+=3)
		printf("%d ", 	tmpbuf[j]);


	unsigned int stat;
	paraddr = (volatile u8 *)EMC_NFPAR0;

	/* Read data */
	REG_EMC_NFINTS = 0x0;
	__nand_ecc_rs_decoding();


	for (j=0;j<512;j++){
		*(char *)dataport = tmpbuf[j];
	}

		/* Set PAR values */
		for (j = 0; j < PAR_SIZE; j++) {
			*paraddr++ = ecc_buf[j];
		}
    struct timespec req, rem;
    struct timeval tv,tv1;
    struct timezone tz;

    gettimeofday(&tv, &tz);

		/* Set PRDY */
		REG_EMC_NFECR |= EMC_NFECR_PRDY;

		/* Wait for completion */
		__nand_ecc_decode_sync();
		__nand_ecc_disable();
    double s;
    gettimeofday(&tv1, &tz);
    s = tv1.tv_sec-tv.tv_sec+0.000001*(tv1.tv_usec-tv.tv_usec);
    printf("\n-- total time consumed: %fs\n",s);

		/* Check decoding */
		stat = REG_EMC_NFINTS;
		
		if (stat & EMC_NFINTS_ERR) {
			//printf("Error occurred\n");
			if (stat & EMC_NFINTS_UNCOR) {
				printf("Uncorrectable ECC error occurred\n");
			}
			else {
				unsigned int errcnt = (stat & EMC_NFINTS_ERRCNT_MASK) >> EMC_NFINTS_ERRCNT_BIT;
				switch (errcnt) {
				case 4: printf("4 errors\n");
					rs_correct(tmpbuf, (REG_EMC_NFERR3 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR3 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				case 3: printf("3 errors\n");
					rs_correct(tmpbuf, (REG_EMC_NFERR2 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR2 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				case 2: printf("2 errors\n");
					rs_correct(tmpbuf, (REG_EMC_NFERR1 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR1 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
				case 1: printf("1 error\n");
					rs_correct(tmpbuf, (REG_EMC_NFERR0 & EMC_NFERR_INDEX_MASK) >> EMC_NFERR_INDEX_BIT, (REG_EMC_NFERR0 & EMC_NFERR_MASK_MASK) >> EMC_NFERR_MASK_BIT);
					break;
				default:  printf("no error\n");
					break;
				}
			}
		}

		for(j=0;j<10;j+=3)
			printf("%d ", 	tmpbuf[j]);
}

