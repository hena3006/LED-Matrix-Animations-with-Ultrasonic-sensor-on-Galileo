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
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/errno.h>
//#include "rdtsc.h"
//static unsigned int gpioLED = 13;
#define DEVICE_NAME "MYDRIVER"




static struct dev		//device structure
{
	struct cdev char_device;
	char *name;                 /* Name of device */
}* my_device;

struct class *class1;
static dev_t DISTANCE_MEA;
int irq; 
int FLAG;
unsigned long long t1;
unsigned long long t2;
int edge=0;
//RDTSC Function
static __inline__ unsigned long long rdtsc(void)
{
  	unsigned hi, lo;
 	 __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
 	 return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

//Interrupt Handler to detect the rising and falling edge
static irqreturn_t   RISINGFUNC(unsigned int irq, void *dev_id)
{
	if(edge==0)
	{
		//printk("Rising edge interrupt\n");
		t1 = rdtsc();
	    	irq_set_irq_type(irq, IRQF_TRIGGER_FALLING);
		FLAG=1;
	    	edge=1;
	}
	else
	{
		//printk("Falling edge interrupt\n");
		t2 = rdtsc();
	    	irq_set_irq_type(irq, IRQF_TRIGGER_RISING);
	    	edge=0;
		FLAG=0;
	}
	return IRQ_HANDLED;
}
	
//Open Function
int my_open(struct inode *i, struct file *f)
{
	
	struct dev *devp;
	devp = container_of(i->i_cdev, struct dev, char_device);
	f->private_data = devp;
	return 0;
}

//Release Function
int my_close(struct inode *i, struct file *f)
{
	struct dev *devp=f->private_data;
	printk(KERN_INFO"\n%s is closing\n", devp->name);
	return 0;
}

//Write function to trigger the gpio pin 13
ssize_t sq_write(struct file *f,const char *buff, size_t count, loff_t *offp)
{
	if(FLAG== 1)
	{
		
		return -EBUSY;
	}
	
	printk("\n1 ");
	gpio_set_value_cansleep(13, 1);
	msleep(20);
	gpio_set_value_cansleep(13, 0);
	return 0;
}

//Read Function to copy the distance to user
ssize_t sq_read(struct file *f, char *m1, size_t count, loff_t *ppos)
{
	int ret;
	unsigned int value;
	unsigned long long difference;
	
	if(FLAG == 1)
	{
		
		return -EBUSY;
	}
	else
	{
		
		difference = t2-t1;
		value = div_u64(difference,400);
			
		ret = copy_to_user((void *)m1, (const void *)&value, sizeof(value));
		
	}
	//printk("pulse_read() End\n");
	return 0;
}

// File operations structure.
static const struct file_operations my_operations = {
	.owner = THIS_MODULE,
	.open = my_open,
	.release = my_close,
	.read	=  sq_read,
	.write = sq_write,
	
};

int __init mydriver_init(void)  //Driver Initialization
{
	int err;
	int result;
	int ret;
	printk(KERN_ALERT "initializing\n");
	//export the gpio pins
	gpio_request(13, "Echo");
	gpio_request(14, "Trigger");
	gpio_request(34, "Mux");
	gpio_request(16, "level");
	gpio_request(77, "Mux");
	gpio_request(76, "Mux");
	
	
	gpio_export(13,false);
	gpio_export(14,false);
	gpio_export(34,false);
	gpio_export(16,false);
	gpio_export(77,false);
	gpio_export(76,false);

	
	printk(" Config done");
	
	 //Request dynamic allocation of a device major number 
	if(alloc_chrdev_region(&DISTANCE_MEA,0,1,DEVICE_NAME)<0)
	{
		printk(KERN_DEBUG "Can't register device\n");
	}
	if((class1=class_create(THIS_MODULE,DEVICE_NAME)) == NULL)
	{
		unregister_chrdev_region(DISTANCE_MEA, 1);
	}

	my_device = (struct dev*) kmalloc(sizeof(struct dev), GFP_KERNEL);
	if (!my_device)
	{
		printk("Bad kmalloc\n");
		return -ENOMEM;
	}
		
		
	//printk("2\n");
	my_device->name="DISTANCE";
	
	if(device_create(class1, NULL, MKDEV(MAJOR(DISTANCE_MEA), MINOR(DISTANCE_MEA)), NULL, "DISTANCE") == NULL)
	{
		class_destroy(class1);
		unregister_chrdev_region(DISTANCE_MEA, 1);
		printk(KERN_DEBUG "Can't register input device\n");
		return -1;
	}
	
		//Connect the file operations with the cdev
	cdev_init(&(my_device->char_device), &my_operations);
	my_device->char_device.owner = THIS_MODULE;
	err=cdev_add(&my_device->char_device,MKDEV(MAJOR(DISTANCE_MEA), MINOR(DISTANCE_MEA)), 1);
	if(err)
	{
		printk("Bad cdev\n");
		return err;
	}
	
	

	printk("created as device");
	//to request IRQ number of the GPIO
	irq = gpio_to_irq(14);
	if(irq < 0)
	{
		printk("Here Gpio %d cannot be used as interrupt",14);
		ret=-EINVAL;
	}
	
	t1=0;
	t2=0;	
	result = request_irq(irq,(irq_handler_t)RISINGFUNC, IRQF_TRIGGER_RISING,"gpio_change_state", NULL); 
	if(result)
	{
		printk("Unable to claim irq %d; error %d\n ", irq, result);
		return 0;
	}
	edge=0;
	printk("\n irq request no. %d ",result);	

	printk("\n irq no. %d ",irq);
	return 0;

}

void __exit mydriver_exit(void)
{
	
	// device_remove_file(gmem_dev_device, &dev_attr_xxx);
	// Release the major number 
	device_destroy(class1, MKDEV(MAJOR(DISTANCE_MEA), MINOR(DISTANCE_MEA)));
	//Destroy driver_class 
	class_destroy(class1);
	unregister_chrdev_region(DISTANCE_MEA,0);
	cdev_del(&my_device->char_device);
	kfree(my_device);
	free_irq(irq,NULL);
	// Unexport the Button GPIO
 	// Free the LED GPIO
	gpio_unexport(13);               
   	gpio_free(13);                    
   	gpio_unexport(14);              
  	gpio_free(14);                     
   	gpio_unexport(34);               
   	gpio_free(34);                      
   	gpio_unexport(16);               
   	gpio_free(16);                                                                
   	gpio_unexport(77);               
   	gpio_free(77);                     
   	gpio_unexport(76);               
   	gpio_free(76);                   
                      
	
	//printk("\n 1");
	printk(KERN_INFO" deleted as device");
	
}

module_init(mydriver_init);
module_exit(mydriver_exit);
MODULE_LICENSE("GPL v2");
