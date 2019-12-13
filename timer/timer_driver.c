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
#include <linux/timer.h>
#include <linux/semaphore.h>

#include <asm/io.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>

#define TIMER_CNT    1 /*number of device num*/
#define TIMER_NAME   "timer_test"

#define CLOSE_CMD       (_IO(0xEF,0x1))
#define OPEN_CMD        (_IO(0xEF,0x2))
#define SETPERIOD_CMD   (_IO(0xEF,0x3))


struct timer_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    int led_gpio;
    int timeperiod;
    struct timer_list mytimer;
    spinlock_t lock;
};

struct timer_dev timer;//timer device


static int led_init(void)
{
    int ret = 0;
    //1.get device node
    timer.nd = of_find_node_by_path("/baseled_test_node");
    if(timer.nd == NULL)
    {
        printk(KERN_EMERG "timer baseled_test_node not find!\r\n");
        return -EINVAL;
    }
    else
    {
        printk(KERN_EMERG "baseled_test_node find !\r\n");
    }

    //2.get gpio property
    timer.led_gpio = of_get_named_gpio(timer.nd, "baseled-gpio", 0);

    if(timer.led_gpio < 0)
    {
        printk(KERN_EMERG "can't get led-gpio!\r\n");
        return -EINVAL;
    }
    printk(KERN_EMERG "led-gpio num = %d\r\n", timer.led_gpio);

    //3.config timer gpio
    gpio_request(timer.led_gpio, "led");
    ret = gpio_direction_output(timer.led_gpio, 1);
    if(ret < 0)
    {
        printk(KERN_EMERG "can't set gpio!\r\n");
    }
    return 0;
}

static int timer_open(struct inode *inode,struct file *filp)
{
    int ret;
    printk(KERN_EMERG "timer_open enter!\n");
    filp->private_data = &timer; //set private_data

    timer.timeperiod = 1000;
    
    ret = led_init();
    if(ret < 0)
    {
        return ret;
    }
    return 0;
}

static ssize_t timer_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
    printk(KERN_EMERG "timer_read enter!\n");
    return 0;
}

static ssize_t timer_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    printk(KERN_EMERG "timer_write enter!\n");
    return 0;
}

static int timer_release(struct inode *inode, struct file *filp)
{
    printk(KERN_EMERG "timer_release enter!\n");
    return 0;
}

static long timer_unlocked_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct timer_dev *dev = (struct timer_dev *)filp->private_data;
    int timeperiod = 0;
    unsigned long flags;

    printk(KERN_EMERG "timer_unlocked_ioctl enter!\n");
    printk(KERN_EMERG "cmd :%d \r\n",cmd);
    switch(cmd)
    {
        case CLOSE_CMD:
        {
            del_timer_sync(&dev->mytimer);
            break;
        }
        case OPEN_CMD:
        {
            spin_lock_irqsave(&dev->lock, flags);
            timeperiod = dev->timeperiod;
            spin_unlock_irqrestore(&dev->lock, flags);
            mod_timer(&dev->mytimer,jiffies + msecs_to_jiffies(timeperiod));
            break;
        }
        case SETPERIOD_CMD:
        {
            spin_lock_irqsave(&dev->lock, flags);
            dev->timeperiod = arg;
            spin_unlock_irqrestore(&dev->lock, flags);
            mod_timer(&dev->mytimer,jiffies + msecs_to_jiffies(timeperiod));
        }
        default:
            break;
    }
    return 0;
}

static struct file_operations timer_fops = {
    .owner = THIS_MODULE,
    .open = timer_open,
    .read = timer_read,
    .write = timer_write,
    .release = timer_release,
    .unlocked_ioctl = timer_unlocked_ioctl,
};

void timer_function(unsigned long arg)
{
    struct timer_dev *dev = (struct timer_dev *)arg;
    static int sta = 1;
    int timeperiod;
    unsigned long flags;

    sta=!sta;
    gpio_set_value(dev->led_gpio, sta);

    /*restart timer*/
    spin_lock_irqsave(&dev->lock, flags);
    timeperiod = dev->timeperiod;
    spin_unlock_irqrestore(&dev->lock, flags);
    mod_timer(&dev->mytimer,jiffies + msecs_to_jiffies(timeperiod));
}

static int __init timer_init(void)
{	
    printk(KERN_EMERG "timer_init enter!\n");
    
    spin_lock_init(&timer.lock);

    /*1.create device ID */
    if(timer.major)
    {
        timer.devid = MKDEV(timer.major, 0);
        register_chrdev_region(timer.devid, TIMER_CNT, TIMER_NAME);
    }
    else
    {
        alloc_chrdev_region(&timer.devid, 0, TIMER_CNT, TIMER_NAME);
        timer.major = MAJOR(timer.devid);
        timer.minor = MINOR(timer.devid);
    }
    printk(KERN_EMERG "nemchrdev major=%d,minor=%d \r\n", timer.major, timer.minor);
	
    /*2.Init cdev */	
    timer.cdev.owner = THIS_MODULE;
    cdev_init(&timer.cdev, &timer_fops);

    /*3.add cdev*/
    cdev_add(&timer.cdev, timer.devid, TIMER_CNT);

    /*4.create class*/
    timer.class = class_create(THIS_MODULE, TIMER_NAME);
    if(IS_ERR(timer.class))
    {
        return PTR_ERR(timer.class);
    }

    /*5.create device*/
    timer.device = device_create(timer.class, NULL, timer.devid, NULL, TIMER_NAME);
    if(IS_ERR(timer.device))
    {
        return PTR_ERR(timer.device);
    }

    /*init timer*/
    init_timer(&timer.mytimer);
    timer.mytimer.function = timer_function;
    timer.mytimer.data = (unsigned long)&timer;
    return 0;
	
}

static void __exit timer_exit(void)
{ 
    printk(KERN_EMERG "timer_exit enter!\n");

    gpio_set_value(timer.led_gpio, 0);
    del_timer_sync(&timer.mytimer);

    /*Uninstall divece*/
	cdev_del(&timer.cdev);//delete cdev
    unregister_chrdev_region(timer.devid,TIMER_CNT);

    device_destroy(timer.class,timer.devid);
    class_destroy(timer.class);
    
}

module_init(timer_init);
module_exit(timer_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("GYY");

