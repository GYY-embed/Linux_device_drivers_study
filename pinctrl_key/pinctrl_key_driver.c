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

#define KEY_CNT    1 /*number of device num*/
#define KEY_NAME   "pinctrl_key"

#define KEYVALUE     0x01   
#define INVAKEY      0xFF


struct key_dev
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
};

struct key_dev key;//key device


static int keyio_init(void)
{
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
    key.key_gpio = of_get_named_gpio(key.nd, "key-gpio", 0);

    if(key.key_gpio < 0)
    {
        printk(KERN_EMERG "can't get key-gpio!\r\n");
        return -EINVAL;
    }
    printk(KERN_EMERG "key-gpio num = %d\r\n", key.key_gpio);

    //3.config key gpio
    gpio_request(key.key_gpio, "key0");
    gpio_direction_input(key.key_gpio);
    return 0;
}

static int key_open(struct inode *inode,struct file *filp)
{
    int ret;
    printk(KERN_EMERG "key_open enter!\n");
    filp->private_data = &key; //set private_data
    ret = keyio_init();
    if( ret <0 )
    {
        return ret;
    }
    return 0;
}

static ssize_t key_read(struct file *filp,char __user *buf,size_t cnt,loff_t *offt)
{
    int ret;
    unsigned char value;
    struct key_dev *dev = filp->private_data;
   // printk(KERN_EMERG "key_read enter!\n");

    if(gpio_get_value(dev->key_gpio) == 0)
    {
        while(!gpio_get_value(dev->key_gpio)) 
            ;
        atomic_set(&dev->keyvalue, KEYVALUE);
    }
    else
    {
        atomic_set(&dev->keyvalue, INVAKEY);
    }
    
    value = atomic_read(&dev->keyvalue);
    ret = copy_to_user(buf, &value, sizeof(value));
    return 0;
}

static ssize_t key_write(struct file *filp,const char __user *buf,size_t cnt,loff_t *offt)
{
    printk(KERN_EMERG "key_write enter!\n");
    return 0;
}

static int key_release(struct inode *inode,struct file *filp)
{
    printk(KERN_EMERG "key_release enter!\n");
    return 0;
}

static struct file_operations key_fops = {
    .owner = THIS_MODULE,
    .open = key_open,
    .read = key_read,
    .write = key_write,
    .release = key_release,
};



static int __init pinctrl_key_init(void)
{	
    printk(KERN_EMERG "key_init enter!\n"); 
    
    atomic_set(&key.keyvalue, INVAKEY); 

 
    /*1.create device ID */
    if(key.major)
    {
        key.devid = MKDEV(key.major, 0);
        register_chrdev_region(key.devid, KEY_CNT, KEY_NAME);
    }
    else
    {
        alloc_chrdev_region(&key.devid, 0, KEY_CNT, KEY_NAME);
        key.major = MAJOR(key.devid);
        key.minor = MINOR(key.devid);
    }
    printk(KERN_EMERG "nemchrdev major=%d,minor=%d \r\n", key.major, key.minor);
	
    /*2.Init cdev */	
    key.cdev.owner = THIS_MODULE;
    cdev_init(&key.cdev, &key_fops);

    /*3.add cdev*/
    cdev_add(&key.cdev, key.devid, KEY_CNT);

    /*4.create class*/
    key.class = class_create(THIS_MODULE, KEY_NAME);
    if(IS_ERR(key.class))
    {
        return PTR_ERR(key.class);
    }

    /*5.create device*/
    key.device = device_create(key.class, NULL, key.devid, NULL, KEY_NAME);
    if(IS_ERR(key.device))
    {
        return PTR_ERR(key.device);
    }

    return 0;
	
}

static void __exit pinctrl_key_exit(void)
{ 
    printk(KERN_EMERG "key_exit enter!\n");

    /*Uninstall divece*/
	cdev_del(&key.cdev);//delete cdev
    unregister_chrdev_region(key.devid,KEY_CNT);

    device_destroy(key.class,key.devid);
    class_destroy(key.class);
    
}

module_init(pinctrl_key_init);
module_exit(pinctrl_key_exit);


MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("GYY");

