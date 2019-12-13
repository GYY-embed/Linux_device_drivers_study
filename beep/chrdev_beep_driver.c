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

#define BEEP_CNT    1 /*number of device num*/
#define BEEP_NAME   "beep"
#define BEEPOFF     0   
#define BEEPON      1


struct beep_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    int beep_gpio;
};

struct beep_dev beep;//beep device


static int beep_open(struct inode *inode,struct file *filp)
{
    printk(KERN_EMERG "beep_open enter!\n");
    filp->private_data = &beep; //set private_data
    return 0;
}

static ssize_t beep_read(struct file *filp,char __user *buf,size_t cnt,loff_t *offt)
{
    printk(KERN_EMERG "beep_read enter!\n");
    return 0;
}

static ssize_t beep_write(struct file *filp,const char __user *buf,size_t cnt,loff_t *offt)
{
    int retvalue;
    unsigned char databuf[1];
    unsigned char beepstat;
    struct beep_dev *dev = filp->private_data;

    retvalue = copy_from_user(databuf, buf, cnt);
    if(retvalue < 0)
    {
        printk(KERN_EMERG "Kernel write failed!\n");
        return -EINVAL;
    }

    beepstat = databuf[0];
    if(beepstat == BEEPON)
    {
        gpio_set_value(dev->beep_gpio, 1);
    }
    else if (beepstat == BEEPOFF)
    {
        gpio_set_value(dev->beep_gpio, 0);
    }

    return 0;
}

static int beep_release(struct inode *inode,struct file *filp)
{
    printk(KERN_EMERG "beep_release enter!\n");
    return 0;
}

static struct file_operations beep_fops = {
    .owner = THIS_MODULE,
    .open = beep_open,
    .read = beep_read,
    .write = beep_write,
    .release = beep_release,
};



static int __init beep_init(void)
{	
    int ret = 0;
    printk(KERN_EMERG "beep_init enter!\n");
    
    //1.get device node
    beep.nd = of_find_node_by_path("/beep_test_node");
    if(beep.nd == NULL)
    {
        printk(KERN_EMERG "beep beep_test_node not find!\r\n");
        return -EINVAL;
    }
    else
    {
        printk(KERN_EMERG "beep_test_node find !\r\n");
    }

    //2.get gpio property
    beep.beep_gpio = of_get_named_gpio(beep.nd, "beep-gpio", 0);

    if(beep.beep_gpio < 0)
    {
        printk(KERN_EMERG "can't get beep-gpio!\r\n");
        return -EINVAL;
    }
    printk(KERN_EMERG "beep-gpio num = %d\r\n", beep.beep_gpio);

    //3.config beep gpio
    ret = gpio_direction_output(beep.beep_gpio, 0);
    if(ret < 0)
    {
        printk(KERN_EMERG "can't set gpio!\r\n");
    }

    /*1.create device ID */
    if(beep.major)
    {
        beep.devid = MKDEV(beep.major, 0);
        register_chrdev_region(beep.devid, BEEP_CNT, BEEP_NAME);
    }
    else
    {
        alloc_chrdev_region(&beep.devid, 0, BEEP_CNT, BEEP_NAME);
        beep.major = MAJOR(beep.devid);
        beep.minor = MINOR(beep.devid);
    }
    printk(KERN_EMERG "nemchrdev major=%d,minor=%d \r\n", beep.major, beep.minor);
	
    /*2.Init cdev */	
    beep.cdev.owner = THIS_MODULE;
    cdev_init(&beep.cdev, &beep_fops);

    /*3.add cdev*/
    cdev_add(&beep.cdev, beep.devid, BEEP_CNT);

    /*4.create class*/
    beep.class = class_create(THIS_MODULE, BEEP_NAME);
    if(IS_ERR(beep.class))
    {
        return PTR_ERR(beep.class);
    }

    /*5.create device*/
    beep.device = device_create(beep.class, NULL, beep.devid, NULL, BEEP_NAME);
    if(IS_ERR(beep.device))
    {
        return PTR_ERR(beep.device);
    }

    return 0;
	
}

static void __exit beep_exit(void)
{ 
    printk(KERN_EMERG "beep_exit enter!\n");

    /*Uninstall divece*/
	cdev_del(&beep.cdev);//delete cdev
    unregister_chrdev_region(beep.devid,BEEP_CNT);

    device_destroy(beep.class,beep.devid);
    class_destroy(beep.class);
    
}

module_init(beep_init);
module_exit(beep_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("GYY");

