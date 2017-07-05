#ifndef __NAND_H__
#define __NAND_H__

extern unsigned int GPIO_BASE;
extern unsigned int EMC_BASE;

extern unsigned int addrport;
extern unsigned int dataport;
extern unsigned int cmdport;

extern int bus;
extern int row;
extern int pagesize;
extern int oobsize;
extern int ppb;

extern int nand_init_4740(unsigned int gbase, unsigned int ebase, unsigned int aport, unsigned int dport, unsigned int cport);
extern int nand_fini_4740(void);
extern unsigned int nand_query_4740(void);
extern int nand_check_badblock(unsigned int blk);
extern int nand_mark_badblock(unsigned int blk);
extern int nand_erase_block(unsigned int blk);
extern int nand_read_page(unsigned char *databuf, unsigned char *oobbuf, unsigned int pageaddr, int ecc_pos);
extern int nand_program_page(unsigned char *databuf, unsigned char *oobbuf, unsigned int pageaddr, int ecc_pos);

#endif /* __NAND_H__ */
