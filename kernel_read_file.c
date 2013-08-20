#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/mm.h>

MODULE_AUTHOR("Kenthy@163.com.");
MODULE_DESCRIPTION("Kernel study and test.");


void fileread(const char * filename)
{
  struct file *filp;
  struct inode *inode;
  mm_segment_t fs;
  off_t fsize;
  char *buf;
  unsigned long magic;
  printk("<1>start....\n");
  filp=filp_open(filename,O_RDONLY,0);
  inode=filp->f_dentry->d_inode; 
 
  magic=inode->i_sb->s_magic;
  printk("<1>file system magic:%li \n",magic);
  printk("<1>super blocksize:%li \n",inode->i_sb->s_blocksize);
  printk("<1>inode %li \n",inode->i_ino);
  fsize=inode->i_size;
  printk("<1>file size:%i \n",(int)fsize);
  buf=(char *) kmalloc(fsize+1,GFP_ATOMIC);

  fs=get_fs();
  set_fs(KERNEL_DS);
  filp->f_op->read(filp,buf,fsize,&(filp->f_pos));
  set_fs(fs);

  buf[fsize]='\0';
  printk("<1>The File Content is:\n");
  printk("<1>%s",buf);


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

  printk("<1>Read File from Kernel.\n");
  fileread(filename);
  filewrite(filename, "kernel write test\n");
  return 0;
}

void cleanup_module()
{
  printk("<1>Good,Bye!\n");
} 
