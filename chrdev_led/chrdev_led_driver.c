#include <linux/init.h>
#include <linux/ide.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/types.h>
/*定义module_param module_param_array的头文件*/
#include <linux/moduleparam.h>
/*定义module_param module_param_array中的参数perm的头文件*/
#include <linux/stat.h>
/*三个字符设备注册函数*/
#include <linux/fs.h>
/*宏定义MKDEV的头文件，MKDEV转换设备号数据类型*/
#include <linux/kdev_t.h>
/*定义字符设备的结构体*/
#include <linux/cdev.h>
/*分配内存空间函数头文件*/
#include <linux/slab.h>
/*包含结构体class以及相关函数的头文件*/
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/errno.h>

#include <asm/io.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>

#define NEWCHRLED_CNT   1 /*number of device num*/
#define NEWCHRLED_NAME  "newchrled"
#define LEDOFF          0   
#define LEDON           1


#define GPL2CON_BASE    (0x11000100)
#define GPL2DAT_BASE    (0x11000104)
#define GPL2PUD_BASE    (0x11000108)

static unsigned long  __iomem *GPL2CON;
static unsigned long  __iomem *GPL2DAT;
static unsigned long  __iomem *GPL2PUD;
volatile unsigned long virt_addr, phys_addr;
//volatile unsigned long *GPL2CON, *GPL2DAT, *GPL2PUD;

struct newchrled_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
};

struct newchrled_dev newchrled;//led device

/*gpl2_ioremap*/
void gpl2_ioremap(void)
{
//    phys_addr=0x11000100;
	//0x11000100是GPL2CON的物理地址
//	virt_addr=ioremap(phys_addr,0x10);
	//指定需要操作的寄存器地址
//	GPL2CON = (unsigned long *)(virt_addr+0x00);
//	GPL2DAT = (unsigned long *)(virt_addr+0x04);
//	GPL2PUD = (unsigned long *)(virt_addr+0x08);

    GPL2CON = (unsigned long *)ioremap(GPL2CON_BASE, 4);
    GPL2DAT = (unsigned long *)ioremap(GPL2DAT_BASE, 4);
    GPL2PUD = (unsigned long *)ioremap(GPL2PUD_BASE, 4);
}
/*gpl2_iounremap*/
//void gpl2_iounremap(void)
//{
   // iounremap(GPL2CON);
    //iounremap(GPL2DAT);
    //iounremap(GPL2PUD);
//}

/*gpl2_config*/
void gpl2_config(void)
{
	*GPL2CON &= 0xFFFFFFF0;
	*GPL2CON |= 0x00000001;
	*GPL2PUD |= 0x0003;	
}
/*gpl2_on*/
void gpl2_on(void)
{
	*GPL2DAT |= 1<<0;
}
/*gpl2_off*/
void gpl2_off(void)
{
	*GPL2DAT &= ~(1<<0);
}


static int led_open(struct inode *inode,struct file *filp)
{
    printk(KERN_EMERG "led_open enter!\n");
    filp->private_data = &newchrled; //set private_data
    return 0;
}

static ssize_t led_read(struct file *filp,char __user *buf,size_t cnt,loff_t *offt)
{
    printk(KERN_EMERG "led_read enter!\n");
    return 0;
}

static ssize_t led_write(struct file *filp,const char __user *buf,size_t cnt,loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char ledstat;

    printk(KERN_EMERG "led_write enter!\n");
    
    retvalue = copy_from_user(databuf, buf, cnt);
    if(retvalue < 0)
    {
        printk(KERN_EMERG "kernel write failed!\r\n");
        return -EFAULT;
    }

    ledstat = databuf[0];

    if(ledstat == LEDON)
    {
        gpl2_on();
        printk(KERN_EMERG "LEDON\r\n");
    }
    else if(ledstat == LEDOFF)
    {
        gpl2_off();
        printk(KERN_EMERG "LEDOFF\r\n");
    }
    else
    {
        printk(KERN_EMERG "Data:%d\r\n",ledstat);
    }
    

    return 0;
}

static int led_release(struct inode *inode,struct file *filp)
{
    printk(KERN_EMERG "led_release enter!\n");
    return 0;
}

static struct file_operations newchrled_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
};



static int __init led_init(void)
{	
    printk(KERN_EMERG "led_init enter!\n");
    
    /*Init LED */
    gpl2_ioremap();
	gpl2_config();
    gpl2_off();

    /*1.create device ID */
    if(newchrled.major)
    {
        newchrled.devid = MKDEV(newchrled.major, 0);
        register_chrdev_region(newchrled.devid, NEWCHRLED_CNT, NEWCHRLED_NAME);
    }
    else
    {
        alloc_chrdev_region(&newchrled.devid, 0, NEWCHRLED_CNT, NEWCHRLED_NAME);
        newchrled.major = MAJOR(newchrled.devid);
        newchrled.minor = MINOR(newchrled.devid);
    }
    printk(KERN_EMERG "nemchrdev major=%d,minor=%d \r\n", newchrled.major, newchrled.minor);
	
    /*2.Init cdev */	
    newchrled.cdev.owner = THIS_MODULE;
    cdev_init(&newchrled.cdev, &newchrled_fops);

    /*3.add cdev*/
    cdev_add(&newchrled.cdev, newchrled.devid, NEWCHRLED_CNT);

    /*4.create class*/
    newchrled.class = class_create(THIS_MODULE, NEWCHRLED_NAME);
    if(IS_ERR(newchrled.class))
    {
        return PTR_ERR(newchrled.class);
    }

    /*5.create device*/
    newchrled.device = device_create(newchrled.class, NULL, newchrled.devid, NULL, NEWCHRLED_NAME);
    if(IS_ERR(newchrled.device))
    {
        return PTR_ERR(newchrled.device);
    }

    return 0;
	
}

static void __exit led_exit(void)
{ 
    printk(KERN_EMERG "led_exit enter!\n");
    //gpl2_iounremap();
    gpl2_off();
    /*Uninstall divece*/
	cdev_del(&newchrled.cdev);//delete cdev
    unregister_chrdev_region(newchrled.devid,NEWCHRLED_CNT);

    device_destroy(newchrled.class,newchrled.devid);
    class_destroy(newchrled.class);
    

}

module_init(led_init);
module_exit(led_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("GYY");

