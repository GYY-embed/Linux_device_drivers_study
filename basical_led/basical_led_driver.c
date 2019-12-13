#include <linux/init.h>
#include <linux/ide.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
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
#include <linux/of_gpio.h>

#include <asm/io.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>

#define LED_CNT    1 /*number of device num*/
#define LED_NAME   "baseled"
#define LEDOFF     0   
#define LEDON      1


struct baseled_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    int baseled_gpio;
};

struct baseled_dev baseled;//baseled device


static int baseled_open(struct inode *inode,struct file *filp)
{
    printk(KERN_EMERG "baseled_open enter!\n");
    filp->private_data = &baseled; //set private_data
    return 0;
}

static ssize_t baseled_read(struct file *filp,char __user *buf,size_t cnt,loff_t *offt)
{
    printk(KERN_EMERG "baseled_read enter!\n");
    return 0;
}

static ssize_t baseled_write(struct file *filp,const char __user *buf,size_t cnt,loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char baseledstat;
    struct baseled_dev *dev = filp->private_data;

    retvalue = copy_from_user(databuf, buf, cnt);
    if(retvalue < 0)
    {
        printk(KERN_EMERG "Kernel write failed!\n");
        return -EINVAL;
    }

    baseledstat = databuf[0];
    if(baseledstat == LEDON)
    {
        gpio_set_value(dev->baseled_gpio, 1);
    }
    else if (baseledstat == LEDOFF)
    {
        gpio_set_value(dev->baseled_gpio, 0);
    }

    return 0;
}

static int baseled_release(struct inode *inode,struct file *filp)
{
    printk(KERN_EMERG "baseled_release enter!\n");
    return 0;
}

static struct file_operations baseled_fops = {
    .owner = THIS_MODULE,
    .open = baseled_open,
    .read = baseled_read,
    .write = baseled_write,
    .release = baseled_release,
};



static int __init baseled_init(void)
{	
    int ret = 0;
    printk(KERN_EMERG "baseled_init enter!\n");
    
    //1.get device node
    baseled.nd = of_find_node_by_path("/baseled_test_node");
    if(baseled.nd == NULL)
    {
        printk(KERN_EMERG "baseled baseled_test_node not find!\r\n");
        return -EINVAL;
    }
    else
    {
        printk(KERN_EMERG "baseled_test_node find !\r\n");
    }

    //2.get gpio property
    baseled.baseled_gpio = of_get_named_gpio(baseled.nd, "baseled-gpio", 0);

    if(baseled.baseled_gpio < 0)
    {
        printk(KERN_EMERG "can't get baseled-gpio!\r\n");
        return -EINVAL;
    }
    printk(KERN_EMERG "baseled-gpio num = %d\r\n", baseled.baseled_gpio);

    //3.config baseled gpio
    ret = gpio_direction_output(baseled.baseled_gpio, 0);
    if(ret < 0)
    {
        printk(KERN_EMERG "can't set gpio!\r\n");
    }

    /*1.create device ID */
    if(baseled.major)
    {
        baseled.devid = MKDEV(baseled.major, 0);
        register_chrdev_region(baseled.devid, LED_CNT, LED_NAME);
    }
    else
    {
        alloc_chrdev_region(&baseled.devid, 0, LED_CNT, LED_NAME);
        baseled.major = MAJOR(baseled.devid);
        baseled.minor = MINOR(baseled.devid);
    }
    printk(KERN_EMERG "nemchrdev major=%d,minor=%d \r\n", baseled.major, baseled.minor);
	
    /*2.Init cdev */	
    baseled.cdev.owner = THIS_MODULE;
    cdev_init(&baseled.cdev, &baseled_fops);

    /*3.add cdev*/
    cdev_add(&baseled.cdev, baseled.devid, LED_CNT);

    /*4.create class*/
    baseled.class = class_create(THIS_MODULE, LED_NAME);
    if(IS_ERR(baseled.class))
    {
        return PTR_ERR(baseled.class);
    }

    /*5.create device*/
    baseled.device = device_create(baseled.class, NULL, baseled.devid, NULL, LED_NAME);
    if(IS_ERR(baseled.device))
    {
        return PTR_ERR(baseled.device);
    }

    return 0;
	
}

static void __exit baseled_exit(void)
{ 
    printk(KERN_EMERG "baseled_exit enter!\n");

    /*Uninstall divece*/
	cdev_del(&baseled.cdev);//delete cdev
    unregister_chrdev_region(baseled.devid,LED_CNT);

    device_destroy(baseled.class,baseled.devid);
    class_destroy(baseled.class);
    
}

module_init(baseled_init);
module_exit(baseled_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("GYY");

