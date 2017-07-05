#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/mm.h>

void fileread(const char * filename)
{
    struct file *filp;
    struct inode *inode;
    mm_segment_t fs;
    off_t fsize;
    char *buf;
    unsigned long magic;
    filp=filp_open(filename,O_RDONLY,0);
    inode=filp->f_dentry->d_inode; 

    magic=inode->i_sb->s_magic;
    fsize=inode->i_size;
    buf=(char *) kmalloc(fsize+1,GFP_ATOMIC);

    fs=get_fs();
    set_fs(KERNEL_DS);
    filp->f_op->read(filp,buf,fsize,&(filp->f_pos));
    set_fs(fs);

    buf[fsize]='\0';

    filp_close(filp,NULL);
}

void filewrite(char* filename, char* data)
{
    struct file *filp;
    mm_segment_t fs;
    filp = filp_open(filename, O_RDWR|O_APPEND, 0644);
    if(IS_ERR(filp))
    {
        printk("open error...\n");
        return;
    }  

    fs=get_fs();
    set_fs(KERNEL_DS);
    filp->f_op->write(filp, data, strlen(data),&filp->f_pos);
    set_fs(fs);
    filp_close(filp,NULL);
}

int init_module()
{
    char *filename="/root/test1.c";

    fileread(filename);
    filewrite(filename, "kernel write test\n");
    return 0;
}

void cleanup_module()
{} 
