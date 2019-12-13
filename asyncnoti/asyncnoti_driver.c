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
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/sched/signal.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/fcntl.h>

#include <asm/io.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>

#define IRQKEY_CNT    1 /*number of device num*/
#define IRQKEY_NAME   "asyncnoti"

#define KEYVALUE     0x01   
#define INVAKEY      0xFF
#define KEY_NUM      1

struct irq_keydesc{
    int gpio;                               //gpio num
    int irqnum;                             //irq num
    unsigned char value;                    //key value
    char name[10];                          //irq name
    irqreturn_t (*handler) (int, void*);    //ird handler
};

struct irqkey_dev
{
    dev_t devid;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    int major;
    int minor;
    struct device_node *nd;
    int key_gpio;
    atomic_t keyvalue;
    atomic_t releasekey;
    struct timer_list timer;
    struct irq_keydesc irqkeydesc[KEY_NUM];
    unsigned char curkeynum;       //current keynum

    wait_queue_head_t r_wait;

    struct fasync_struct *async_queue;
};

struct irqkey_dev key;//key device

//key irq_handler
static irqreturn_t key0_handler(int irq, void *dev_id)
{
    struct irqkey_dev *dev = (struct irqkey_dev *)dev_id;

    dev->curkeynum = 0;
    dev->timer.data = (volatile long)dev_id;
    mod_timer(&dev->timer, jiffies + msecs_to_jiffies(10));
    return 1;
}
//timer function 
void timer_function(unsigned long arg)
{
    unsigned char value;
    unsigned char num;
    struct irq_keydesc *keydesc;
    struct irqkey_dev *dev = (struct irqkey_dev *)arg;

    num = dev->curkeynum;
    keydesc = &dev->irqkeydesc[num];
    value = gpio_get_value(keydesc->gpio);
    if(value == 0)
    {
        atomic_set(&dev->releasekey, keydesc->value);
    }
    else
    {
        atomic_set(&dev->keyvalue, 0x80 | keydesc->value);
        atomic_set(&dev->releasekey, 1);
    }

    //一次完成的按键过程
    if(atomic_read(&dev->releasekey))
    {
        if(dev->async_queue)
            kill_fasync(&dev->async_queue, SIGIO, POLL_IN);
    }
    
}


static int keyio_init(void)
{
    char name[10];
    int ret;
    //1.get device node
    key.nd = of_find_node_by_path("/key_test_node");
    if(key.nd == NULL)
    {
        printk(KERN_EMERG "key key_test_node not find!\r\n");
        return -EINVAL;
    }
    else
    {
        printk(KERN_EMERG "key_test_node find !\r\n");
    }

    //2.get gpio property
    key.irqkeydesc[0].gpio = of_get_named_gpio(key.nd, "key-gpio", 0);
    if( key.irqkeydesc[0].gpio < 0)
    {
        printk(KERN_EMERG "can't get key-gpio!\r\n");
        return -EINVAL;
    }
    printk(KERN_EMERG "key-gpio num = %d\r\n", key.irqkeydesc[0].gpio);

    //3.config key gpio and set interrupt
    printk(KERN_EMERG "OK1\r\n");
    memset(key.irqkeydesc[0].name,0,sizeof(name));
    printk(KERN_EMERG "OK2\r\n");
    sprintf(key.irqkeydesc[0].name, "KEY0");
    printk(KERN_EMERG "OK3\r\n");
    gpio_request(key.irqkeydesc[0].gpio, "key0");
    printk(KERN_EMERG "OK4\r\n");
    gpio_direction_input(key.irqkeydesc[0].gpio);
    printk(KERN_EMERG "OK5\r\n");
    key.irqkeydesc[0].irqnum = irq_of_parse_and_map(key.nd, 0);
    printk(KERN_EMERG "OK6\r\n");
    //gpio_to_irq(key.irqkeydesc[0].gpio);
    printk(KERN_EMERG "irqnum : %d\r\n",key.irqkeydesc[0].irqnum);
    
    //4.request irq
    key.irqkeydesc[0].handler = key0_handler;
    key.irqkeydesc[0].value = KEYVALUE;

    ret = request_irq(key.irqkeydesc[0].irqnum,
				key.irqkeydesc[0].handler,
				IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING,
				key.irqkeydesc[0].name, 
				&key);
    
    //5.create timer
    init_timer(&key.timer);
    key.timer.function = timer_function;

    init_waitqueue_head(&key.r_wait);
    return 0;
}

static int key_open(struct inode *inode,struct file *filp)
{
    printk(KERN_EMERG "key_open enter!\n");
    filp->private_data = &key; 
    return 0;
}

static ssize_t key_read(struct file *filp,char __user *buf,size_t cnt,loff_t *offt)
{
    int ret;
    unsigned char value = 0;
    unsigned char release = 0;
    struct irqkey_dev *dev = (struct irqkey_dev *)filp->private_data;

#if 0
    ret = wait_event_interruptible(dev->r_wait, atomic_read(&dev->releasekey));
    if(ret)
    {
        goto wait_error;
    }

#endif
    if(filp->f_flags & O_NONBLOCK)
    {
        if(atomic_read(&dev->releasekey) == 0)
            return -EAGAIN;
    }
    else
    {
       ret = wait_event_interruptible(dev->r_wait, atomic_read(&dev->releasekey));
        if(ret)
        {
            goto wait_error;
        }
    }

    value = atomic_read(&dev->keyvalue);
    release = atomic_read(&dev->releasekey);
    if(release)
    {
        if(value & 0x80)
        {
            value &= ~0x80;
            ret = copy_to_user(buf, &value, sizeof(value));
        }
        else
        {
            return -EINVAL;
        }
        atomic_set(&dev->releasekey, 0);
    }   
    else
    {
        return -EINVAL;
    }
     
    return 0;
wait_error:
    return ret;
}

static ssize_t key_write(struct file *filp,const char __user *buf,size_t cnt,loff_t *offt)
{
    printk(KERN_EMERG "key_write enter!\n");
    return 0;
}

static int key_fasync(int fd, struct file *filp, int on)
{
    printk(KERN_EMERG "key_fasync enter!\n");
    struct irqkey_dev *dev = (struct irqkey_dev *)filp->private_data;

    return fasync_helper(fd, filp, on, &dev->async_queue);
}

static int key_release(struct inode *inode,struct file *filp)
{
    printk(KERN_EMERG "key_release enter!\n");
    return key_fasync(-1, filp, 0);
}

unsigned int key_poll(struct file *filp, struct poll_table_struct *wait)
{
    unsigned int mask = 0;
    struct irqkey_dev *dev = (struct irqkey_dev *)filp->private_data;

    poll_wait(filp, &dev->r_wait, wait);

    if(atomic_read(&dev->releasekey))//按键按下
    {
        mask = POLLIN | POLLRDNORM; //返回PLLIN
    }
    return mask;
}


static struct file_operations key_fops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .read = key_read,
    .write = key_write,
    .release = key_release,
    .poll = key_poll,
    .fasync = key_fasync,
};



static int __init asyncnoti_init(void)
{	
    printk(KERN_EMERG "key_init enter!\n"); 
    
    atomic_set(&key.keyvalue, INVAKEY); 

 
    /*1.create device ID */
    if(key.major)
    {
        key.devid = MKDEV(key.major, 0);
        register_chrdev_region(key.devid, IRQKEY_CNT, IRQKEY_NAME);
    }
    else
    {
        alloc_chrdev_region(&key.devid, 0, IRQKEY_CNT, IRQKEY_NAME);
        key.major = MAJOR(key.devid);
        key.minor = MINOR(key.devid);
    }
    printk(KERN_EMERG "nemchrdev major=%d,minor=%d \r\n", key.major, key.minor);
	
    /*2.Init cdev */	
    key.cdev.owner = THIS_MODULE;
    cdev_init(&key.cdev, &key_fops);

    /*3.add cdev*/
    cdev_add(&key.cdev, key.devid, IRQKEY_CNT);

    /*4.create class*/
    key.class = class_create(THIS_MODULE, IRQKEY_NAME);
    if(IS_ERR(key.class))
    {
        return PTR_ERR(key.class);
    }

    /*5.create device*/
    key.device = device_create(key.class, NULL, key.devid, NULL, IRQKEY_NAME);
    if(IS_ERR(key.device))
    {
        return PTR_ERR(key.device);
    }

    /*6.init key*/
    atomic_set(&key.keyvalue, INVAKEY);
    atomic_set(&key.releasekey, 0);
    keyio_init();
    return 0;
	
}

static void __exit asyncnoti_exit(void)
{ 
    printk(KERN_EMERG "key_exit enter!\n");

    /*delete timer*/
    del_timer_sync(&key.timer);
    /*release gpio*/
    gpio_free(key.irqkeydesc[0].gpio);
    /*release interrupt*/
    printk(KERN_EMERG "irq_num = %d\r\n",key.irqkeydesc[0].irqnum);
    free_irq(key.irqkeydesc[0].irqnum,&key);

    /*Uninstall divece*/
	cdev_del(&key.cdev);//delete cdev
    unregister_chrdev_region(key.devid,IRQKEY_CNT);

    device_destroy(key.class,key.devid);
    class_destroy(key.class);
    
}

module_init(asyncnoti_init);
module_exit(asyncnoti_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("GYY");

