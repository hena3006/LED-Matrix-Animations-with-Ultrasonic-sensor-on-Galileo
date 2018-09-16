#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/param.h>
#include <linux/list.h>
#include <linux/semaphore.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/delay.h>	
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/spi/spi.h>
//#include <linux/spi/spidev.h>
#include <linux/kthread.h>
#include <linux/delay.h>

#define DRIVER_NAME 		"DISPLAY"
#define DEVICE_NAME 		"DISPLAY"
#define DEVICE_CLASS_NAME	 "DISPLAY"
#define CONFIG 1
#define MINOR_NUMBER    0
#define MAJOR_NUMBER    154 /* assigned */



struct spidev_data {
	dev_t                   devt;
	struct spi_device       *spi;
	//char pattern_buffer[10][8];
	//unsigned int sequence_buffer[10][2];
};


static struct spidev_data *message;
static struct class *class1;


uint8_t array[12][24];
uint8_t pattern[10];
static struct spi_message mes;
uint8_t arraytx[2];
struct spi_transfer tx = {

		.tx_buf = arraytx,
		.rx_buf = 0,
		.len = 2,
		.delay_usecs = 1,
		.speed_hz = 10000000,
		.bits_per_word = 8,
		.cs_change = 1,
	};
int i;
int j;
int k;
int z;
//kernel thread for LED display according to the pattern array
int thread_spi(void *data)
{
	
	printk("\n hello world");
	/*for(i=0;i<10;i++)
	{
			printk(" %d ",pattern[i]);
	}*/
	
	for(i=0;i<10;i=i+2)
	{
	
	z=pattern[i];
	j=i+1;
	
	for(k=0;k<24;k=k+2)
	{
	arraytx[0] = array[z][k];
    	arraytx[1] = array[z][k+1];
	//functionsto send data on SPI bus
	spi_message_init(&mes);
	spi_message_add_tail((void *)&tx, &mes);
	gpio_set_value(15,0);
	spi_sync(message->spi, &mes);
	gpio_set_value(15,1);
	}
	//printk(" %d ", pattern[0]);
	msleep(j*20);
	
	}
	
	printk(" bye ");
	
	return 0;	
}
//open function to set the directions and values of all gpio of SPI
int spi_open(struct inode *i, struct file *f)
{
	
	gpio_export(24,true);
	gpio_export(25,true);
	gpio_export(30,true);
	gpio_export(31,true);
	gpio_export(42,true);
	gpio_export(43,true);
	gpio_export(46,true);
	gpio_export(44,true);
	gpio_export(72,true);
	gpio_export(15,true);
	gpio_export(5,true);
	gpio_export(7,true);

	gpio_direction_output(5,1);
	gpio_set_value(5,1);
	gpio_direction_output(7,1);
	gpio_set_value(7,1);
	gpio_direction_output(46,1);
	gpio_set_value(46,1);
	gpio_direction_output(30,1);
	gpio_set_value(30,0);
	gpio_direction_output(31,1);
	gpio_set_value(31,0);
	gpio_set_value(72,0);
	gpio_direction_output(24,1);
	gpio_set_value(24,0);
	gpio_direction_output(25,1);
	gpio_set_value(25,0);
	gpio_direction_output(44,1);
	gpio_set_value(44,1);
	gpio_direction_output(42,1);
	gpio_set_value(42,0);
	gpio_direction_output(43,1);
	gpio_set_value(43,0);
	gpio_direction_output(15,1);
	gpio_set_value(15,0);

	printk(KERN_INFO" is opening\n");
	return 0;
}

//close function to free all the gpio's
int spi_close(struct inode *i, struct file *f)
{
	
	gpio_unexport(24);
	gpio_free(24);   
	gpio_unexport(25);
	gpio_free(25);   
	gpio_unexport(30);
	gpio_free(30);   
	gpio_unexport(31);
	gpio_free(31);   
	gpio_unexport(42);
	gpio_free(42);   
	gpio_unexport(43);
	gpio_free(43);   
	gpio_unexport(46);
	gpio_free(46);   
	gpio_unexport(44);
	gpio_free(44);   
	gpio_unexport(72);
	gpio_free(72);   
	gpio_unexport(15);
	gpio_free(15);   
	gpio_unexport(5);
	gpio_free(5);   
	gpio_unexport(7);
	gpio_free(7);   
	printk(KERN_INFO"\nis closing");
	return 0;
}
//write function which runs the thread according to the pattern sequence from the user
ssize_t spiled_write(struct file *f,const char *m1, size_t count, loff_t *offp)
{
	
	uint8_t sequence[10];
	struct task_struct *task;
	copy_from_user((void *)&sequence, (void * __user)m1, sizeof(sequence));
	for(j=0;j<10;j++)
	{
			pattern[j] = sequence[j];
			//printk(" %d ",pattern[j]);
	}
	task = kthread_run(&thread_spi, (void *)pattern,"kthread_spi_led");
	msleep(1500);		//sleep is used in this function instead od giving it in user space becaunce kernel was panicking
return 0;


	
}
//IOCTL function to get the pattern buffer from the user
static long spiled_ioctl(struct file *f,unsigned int cmd, unsigned long count)  
{

	
	uint8_t write[12][24];

	int retValue;
   	 printk("ioctl Start\n");
	retValue = copy_from_user((void *)&write,(void *)count, sizeof(write));
	if(retValue != 0)
	{
		printk("Failure : %d number of bytes that could not be copied.\n",retValue);
	}
	for(i=0;i<12;i++)
	{
	for(j=0;j<24;j++)
	{
			array[i][j] = write[i][j];
			//printk(" %d ",array[i][j]);
	}
	}
	printk("ioctl End\n");
	return 0;
}


static const struct file_operations spi_operations = {

	.owner = THIS_MODULE,
	.open = spi_open,
	.release = spi_close,
	.write	= spiled_write,
	.unlocked_ioctl = spiled_ioctl,
	
};

static int spi_probe(struct spi_device *spi)
{
	//struct spidev_data *spidev;
	int status = 0;
	struct device *dev;

	/* Allocate driver data */
	message = kzalloc(sizeof(*message), GFP_KERNEL);
	if(!message)
	{
		return -ENOMEM;
	}

	/* Initialize the driver data */
	message->spi = spi;

	message->devt = MKDEV(MAJOR_NUMBER, MINOR_NUMBER);

    	dev = device_create(class1, &spi->dev, message->devt, message, DEVICE_NAME);

    if(dev == NULL)
    {
		printk("Device Creation Failed\n");
		kfree(message);
		return -1;
	}
	printk("SPI LED Driver Probed.\n");
	return status;
}

static int spi_remove(struct spi_device *spi)
{
	device_destroy(class1, message->devt);
	kfree(message);
	printk("SPI LED Driver Removed.\n");
	return 0;
}

struct spi_device_id leddeviceid[] = {
{"spidev",0},
{}
};

static struct spi_driver spi_led_driver = {
         .driver = {
                 .name =         "spidev",
                 .owner =        THIS_MODULE,
         },
	 .id_table =   leddeviceid,
         .probe =        spi_probe,
         .remove =       spi_remove,
		
};

int __init spidriver_init(void)
{
	
	int retValue;
	
	//Register the Device
	retValue = register_chrdev(MAJOR_NUMBER, DEVICE_NAME, &spi_operations);
	if(retValue < 0)
	{
		printk("Device Registration Failed\n");
		return -1;
	}
	
	//Create the class
	class1 = class_create(THIS_MODULE, DEVICE_CLASS_NAME);
	if(class1 == NULL)
	{
		printk("Class Creation Failed\n");
		unregister_chrdev(MAJOR_NUMBER, spi_led_driver.driver.name);
		return -1;
	}
	
	//Register the Driver
	retValue = spi_register_driver(&spi_led_driver);
	if(retValue < 0)
	{
		printk("Driver Registraion Failed\n");
		class_destroy(class1);
		unregister_chrdev(MAJOR_NUMBER, spi_led_driver.driver.name);
		return -1;
	}
	
	printk("SPI LED Driver Initialized.\n");
	return 0;
}

void __exit spidriver_exit(void)
{
	spi_unregister_driver(&spi_led_driver);
	class_destroy(class1);
	unregister_chrdev(MAJOR_NUMBER,spi_led_driver.driver.name);
	printk("SPI LED Driver Uninitialized.\n");
}



module_init(spidriver_init);
module_exit(spidriver_exit);
MODULE_LICENSE("GPL v2");
