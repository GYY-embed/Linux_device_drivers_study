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
#include <linux/of.h>
#include <linux/of_address.h>
#include <asm/io.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>

#define NEWCHRLED_CNT   1 /*number of device num*/
#define NEWCHRLED_NAME  "dtsled"
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

struct dtsled_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
};

struct dtsled_dev dtsled;//led device

/*gpl2_ioremap*/
void gpl2_ioremap(void)
{
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
    filp->private_data = &dtsled; //set private_data
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

static struct file_operations dtsled_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .read = led_read,
    .write = led_write,
    .release = led_release,
};



static int __init led_init(void)
{	
    u32 val = 0;
    int ret;
    u32 regdata[14];
    const char *str;
    struct property *proper;

    printk(KERN_EMERG "led_init enter!\n");
    


    /*Get Device Tree prorerty data*/
    /*1.Get device node:testled*/
    dtsled.nd = of_find_node_by_path("/testled");
    if(dtsled.nd == NULL)
    {
        printk(KERN_EMERG "testled node not find!\r\n");
        return -EINVAL;
    }
    else
    {
        printk(KERN_EMERG "testled node find!\r\n");
    }
    
    /*2.Get compatible prorerty data*/
    proper = of_find_property(dtsled.nd, "compatible", NULL);
    if(proper == NULL)
    {
        printk(KERN_EMERG "compatible prorerty find failed!\r\n");
    }
    else
    {
         printk(KERN_EMERG "compatible = %s\r\n",(char*)proper->value);
    }

    /*3.Get status prorerty data*/
    ret = of_property_read_string(dtsled.nd, "status", &str);
    if(ret < 0)
    {
        printk(KERN_EMERG "status read failed!\r\n");
    }
    else
    {
        printk(KERN_EMERG "status = %s\r\n",str);
    }

    /*4.Get reg property data*/
    ret = of_property_read_u32_array(dtsled.nd, "reg", regdata, 6);
    if(ret < 0)
    {
        printk(KERN_EMERG "reg prorerty read failed!\r\n");
    }
    else
    {
        u8 i=0;
        printk(KERN_EMERG "reg data:\r\n");
        for(i=0;i<6;i++)
        {
            printk(KERN_EMERG "%#x ",regdata[i]);
        }
        printk(KERN_EMERG "\r\n");
    }
    
    /*Init LED */
    //gpl2_ioremap();
	GPL2CON = of_iomap(dtsled.nd, 0);
    GPL2DAT = of_iomap(dtsled.nd, 1);
    GPL2PUD = of_iomap(dtsled.nd, 2);
    gpl2_config();
    gpl2_off();

    /*1.create device ID */
    if(dtsled.major)
    {
        dtsled.devid = MKDEV(dtsled.major, 0);
        register_chrdev_region(dtsled.devid, NEWCHRLED_CNT, NEWCHRLED_NAME);
    }
    else
    {
        alloc_chrdev_region(&dtsled.devid, 0, NEWCHRLED_CNT, NEWCHRLED_NAME);
        dtsled.major = MAJOR(dtsled.devid);
        dtsled.minor = MINOR(dtsled.devid);
    }
    printk(KERN_EMERG "nemchrdev major=%d,minor=%d \r\n", dtsled.major, dtsled.minor);
	
    /*2.Init cdev */	
    dtsled.cdev.owner = THIS_MODULE;
    cdev_init(&dtsled.cdev, &dtsled_fops);

    /*3.add cdev*/
    cdev_add(&dtsled.cdev, dtsled.devid, NEWCHRLED_CNT);

    /*4.create class*/
    dtsled.class = class_create(THIS_MODULE, NEWCHRLED_NAME);
    if(IS_ERR(dtsled.class))
    {
        return PTR_ERR(dtsled.class);
    }

    /*5.create device*/
    dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, NEWCHRLED_NAME);
    if(IS_ERR(dtsled.device))
    {
        return PTR_ERR(dtsled.device);
    }

    return 0;
	
}

static void __exit led_exit(void)
{ 
    printk(KERN_EMERG "led_exit enter!\n");
    //gpl2_iounremap();
    gpl2_off();
    /*Uninstall divece*/
	cdev_del(&dtsled.cdev);//delete cdev
    unregister_chrdev_region(dtsled.devid,NEWCHRLED_CNT);

    device_destroy(dtsled.class,dtsled.devid);
    class_destroy(dtsled.class);
    

}

module_init(led_init);
module_exit(led_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("GYY");

