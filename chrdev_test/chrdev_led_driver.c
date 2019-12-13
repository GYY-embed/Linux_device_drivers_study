#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>>



#define CHRDEVBASH_MAJOR    200             /*主设备号*/
#define CHRDEVBASH_NAME     "chrdevbase"    /*设备名*/

static char readbuf[100];               /*read buffer*/
static char writebuf[100];              /*write buffer*/
static char kerneldata[] = {"kernel data!"};   

static int chrdevbase_open(struct inode *inode,struct file *filp)
{
    printk(KERN_EMERG "chrdevbase open!\n");
    return 0;
}

static ssize_t chrdevbase_read(struct file *filp,char __user *buf,size_t cnt,loff_t *offt)
{
    int retvalue = 0;

     printk(KERN_EMERG "chrdevbase read!\n");
     /*copy to user zone*/
     memcpy(readbuf, kerneldata, sizeof(kerneldata)) ;
     retvalue = copy_to_user(buf, readbuf, cnt);
     if(retvalue == 0)
     {
         printk(KERN_EMERG "kernel send data ok!\n");
     }
     else
     {
        printk(KERN_EMERG "kernel send data failed!\n");
     }
     
    return 0;
}

static ssize_t chrdevbase_write(struct file *filp,const char __user *buf,size_t cnt,loff_t *offt)
{
    int retvalue = 0;

    printk(KERN_EMERG "chrdevbase write!\n");
    /*get data from user zone and print*/
    retvalue = copy_from_user(writebuf, buf, cnt);
    if(retvalue == 0)
    {
        printk(KERN_EMERG "kernel receivedata:%s\r\n",writebuf);
    }
    else
    {
        printk(KERN_EMERG "kernel receivedata failed!\r\n");
    }
    
    return 0;
}

static int chrdevbase_release(struct inode *inode,struct file *filp)
{
    printk(KERN_EMERG "chrdevbase release!\n");
    return 0;
}

static struct file_operations chrdevbase_fops = {
    .owner = THIS_MODULE,
    .open = chrdevbase_open,
    .read = chrdevbase_read,
    .write = chrdevbase_write,
    .release = chrdevbase_release,
};


static int __init chardevbase_init(void)
{
    int retvalue = 0;

    /*register char device driver*/
    retvalue = register_chrdev(CHRDEVBASH_MAJOR,CHRDEVBASH_NAME,&chrdevbase_fops);

    if(retvalue < 0)
    {
        printk(KERN_EMERG "chrdevbase driver register failed!\n");
    }
    printk(KERN_EMERG "chrdevbase driver register success!\n");
    return 0;
}


static void __exit chardevbase_exit(void)
{
    unregister_chrdev(CHRDEVBASH_MAJOR,CHRDEVBASH_NAME);
    printk("chrdevbase driver unregister success!\n");
}


module_init(chardevbase_init);
module_exit(chardevbase_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("GYY");