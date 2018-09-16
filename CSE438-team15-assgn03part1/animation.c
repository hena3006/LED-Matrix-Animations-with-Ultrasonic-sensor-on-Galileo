#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <linux/input.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <time.h>
#include <poll.h>
#include <sched.h> 
#include <pthread.h>
#include <inttypes.h>
#include <stropts.h>
#include "Gpio_func.h"
#include "Gpio_func.c"
#include "rdtsc.h"
#define Driver "/dev/spidev1.0"

int realtimedistance = 0;
int pastdistance = 0;
int direction = 0;
int fd1;
uint8_t array[2];
pthread_mutex_t distance_lock = PTHREAD_MUTEX_INITIALIZER;

uint64_t array1 [] = {
	
		0x0C, 0x01,	
		0x09, 0x00,	
		//0xFF, 0x00,	
		0x0A, 0x0F,
		0x0B, 0x07,
		0x01, 0x06,
		0x02, 0x06,	
		0x03, 0x06,
		0x04, 0x83,
		0x05, 0x7F,
		0x06, 0x24,
		0x07, 0x22,
		0x08, 0x63,
		//0x0C, 0x00,
};

uint64_t array2 [] = {

		0x0C, 0x01,	
		0x09, 0x00,	
		//0xFF, 0x00,	
		0x0A, 0x0F,
		0x0B, 0x07,
		0x01, 0x06,
		0x02, 0x06,	
		0x03, 0x06,
		0x04, 0x03,
		0x05, 0x7F,
		0x06, 0xA2,
		0x07, 0x32,
		0x08, 0x16,
};

uint64_t array3 [] = {

		//0x0F, 0x00,	
		0x0C, 0x01,
		0x09, 0x00,
		0x0A, 0x0F,
		0x0B, 0x07,
		0x01, 0x60,
		0x02, 0x60,	
		0x03, 0x60,
		0x04, 0xC1,
		0x05, 0xFE,
		0x06, 0x24,
		0x07, 0x44,
		0x08, 0xC6,
};

uint64_t array4 [] = {

		//0x0F, 0x00,	
		0x0C, 0x01,
		0x09, 0x00,
		0x0A, 0x0F,
		0x0B, 0x07,
		0x01, 0x60,
		0x02, 0x60,	
		0x03, 0x60,
		0x04, 0xC0,
		0x05, 0xFE,
		0x06, 0x45,
		0x07, 0x4C,
		0x08, 0x68,
};



void sensorGpio(void) 			//configration Distance sensor
{
	gpio_unexport(13);
	gpio_unexport(14);
	gpio_unexport(34);
	gpio_unexport(16);
	gpio_unexport(77);
	gpio_unexport(76);
	gpio_unexport(64);

	gpio_export(13);
	gpio_export(14);
	gpio_export(34);
	gpio_export(16);
	gpio_export(77);
	gpio_export(76);
	gpio_export(64);
	
	gpio_set_dir(13,1);
	gpio_set_dir(14,0);
	gpio_set_dir(34,1);
	gpio_set_dir(16,1);

	gpio_set_value(13,0);
	gpio_set_value(14,0);
	gpio_set_value(34,0);
	gpio_set_value(77,0);
	gpio_set_value(76,0);
	gpio_set_value(64,0);

	gpio_set_value(16,1);
	gpio_set_dir(14,0);
}

void* displayFunction(void* parameters){	//to display the animation on the LED which depends on the distance

int i=0,fd,ret;
	struct spi_ioc_transfer tr = {

		.tx_buf = (unsigned long)array,
		.rx_buf = 0,
		.len = sizeof(array),
		.delay_usecs = 1,
		.speed_hz = 10000000,
		.bits_per_word = 8,
		.cs_change = 1,
	};
	
	fd=open(Driver,0);
	

	while(1) {

		if(direction == 1) {
			for(i = 0; i < 24;i = i + 2) {

				array[0] = array1[i];
				array[1]= array1[i+1];
				write(fd1,"0",1);
				ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
				if(ret<0) {
					printf("writting error");
				}
				write(fd1,"1",1);
			}

			//sleep(5);
			usleep((int)realtimedistance*3600);
			for(i = 0;i < 24;i = i + 2) {

				array[0] = array2[i];
				array[1]= array2[i+1];
				write(fd1,"0",1);
				ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
				if(ret<0) {
					printf("writting error");
				}
				write(fd1,"1",1);
			}

			usleep((int)realtimedistance*3600);
		}

		if(direction == -1) {

			for(i = 0; i < 24;i = i + 2) {

				array[0] = array3[i];
				array[1]= array3[i+1];
				write(fd1,"0",1);
				ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
				if(ret<0) {
					printf("writting error");
				}
				write(fd1,"1",1);
			}

			//sleep(5);
			usleep((int)realtimedistance*2500);
			for(i = 0;i < 24;i = i + 2) {

				array[0] = array4[i];
				array[1]= array4[i+1];
				write(fd1,"0",1);
				ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
				if(ret<0) {
					printf("writting error");
				}
				write(fd1,"1",1);
			}

			usleep((int)realtimedistance*2500);
		}

	
	}
	return 0;

}

void* distanceFunction(void* parameters) {		//to measure the Time stamp counter for the pulse at echo pin and calcute the distance of the object

	char* buf[2];
	int fd, poll_return;
	long timeout = 2000;
	double timedifference;
	double tstampfalling = 0;
	double tstamprising = 0;
	double interval;
	double threshold;
	struct pollfd fds;
	
	sensorGpio();
	fd = open("/sys/class/gpio/gpio14/edge", O_WRONLY);
	fds.fd = open("/sys/class/gpio/gpio14/value", O_RDONLY);
	fds.events = POLLPRI|POLLERR; 	
	fds.revents=0;
back:	while(1)
	{
		tstampfalling=0;
		tstamprising=0;
		lseek(fds.fd, 0, SEEK_SET);
		// check for rising pulse at the echo pin		
		write(fd, "rising", sizeof("rising"));
		// send the trigger pulse for 20 ms
		gpio_set_value(13,1);
		usleep(20);
		gpio_set_value(13,0);

		// poll for rising event

		poll_return = poll(&fds, 1, timeout);  
		
		if(poll_return > 0) {
			
			if (fds.revents & POLLPRI) {		
				tstamprising = rdtsc();
				read(fds.fd, buf, 1);
			}
			else
			{
				printf("Error Rising Edge\n");
				goto back;
			}
		}
		
		lseek(fds.fd, 0, SEEK_SET);
		// check for a falling pulse if a rising pulse is detected
		write(fd, "falling", sizeof("falling"));
		// poll for falling event
		poll_return = poll(&fds, 1, timeout);

		if(poll_return > 0) {
			
			if(fds.revents & POLLPRI) {
				tstampfalling = rdtsc();
				read(fds.fd, buf, 1);
			}
			else
			{
				printf("\nError Falling Edge");
				goto back;
			}
		}
		//finding the direction based of the distance calculated
		timedifference = (long)(tstampfalling - tstamprising);	
		//prev_distance = distance;	
		pthread_mutex_lock(&distance_lock);	
		realtimedistance = ((timedifference *0.017) / (400));
		
		if(realtimedistance > 350) { 
			realtimedistance = 350;
		}
		else if(realtimedistance < 15) {
			realtimedistance = 15;
		}
		
		interval = realtimedistance - pastdistance;

       		threshold = realtimedistance / 10.0;
       		if(interval > threshold)
        	{
            		direction=-1;
            
        	}
        	else if(interval < -threshold)
        	{
           		 direction=1;
            
        	}
		
		pastdistance = realtimedistance;
		
		printf("%d cm %d \n", realtimedistance, direction);
		pthread_mutex_unlock(&distance_lock);	

		usleep(100000);	
	}
	return 0;
}

void gpiosetup(void) {		//for the pin setup for all pins used by LED Matrix
	int i;
	int pins[12] ={24,25,30,31,42,43,46,44,72,15,5,7};
	for(i=0;i<12;i++) {
		FILE *fp = fopen("/sys/class/gpio/export","w");
    	fprintf(fp,"%d",pins[i]);
    	fclose(fp);
	}

	int fd,ret;

	fd = open("/sys/class/gpio/gpio5/direction", O_WRONLY);
	ret=write(fd, "out", sizeof("out"));	
	if(ret<0) {
	printf("\n error in 5");
	}
	close(fd);		
	
	fd = open("/sys/class/gpio/gpio5/value", O_WRONLY);
	ret=write(fd, "1", sizeof("1"));
	if(ret<0) {
	printf("\n error 5");
	}
	close(fd);

	fd = open("/sys/class/gpio/gpio7/direction", O_WRONLY);
	ret=write(fd, "out", sizeof("out"));	
	if(ret<0) {
	printf("\n error in 7");
	}
	close(fd);		
	
	fd = open("/sys/class/gpio/gpio7/value", O_WRONLY);
	ret=write(fd, "1", sizeof("1"));
	if(ret<0) {
	printf("\n error 7");
	}
	close(fd);	

	fd = open("/sys/class/gpio/gpio46/direction", O_WRONLY);
	ret=write(fd, "out", sizeof("out"));	
	if(ret<0) {
	printf("\n error 30");
	}
	close(fd);		
	
	fd = open("/sys/class/gpio/gpio46/value", O_WRONLY);
	ret=write(fd, "1", sizeof("1"));
	if(ret<0) {
	printf("\n error 46");
	}
	close(fd);
	
	fd = open("/sys/class/gpio/gpio30/direction", O_WRONLY);
	ret=write(fd, "out", sizeof("out"));	
	if(ret<0) {
	printf("\n error 30");
	}
	close(fd);	
	
	fd = open("/sys/class/gpio/gpio30/value", O_WRONLY);
	ret=write(fd, "0", sizeof("0"));	
	if(ret<0) {
	printf("\n error in value 30");
	}
	close(fd);


	fd = open("/sys/class/gpio/gpio31/direction", O_WRONLY);
	ret=write(fd, "out", sizeof("out"));	
	if(ret<0) {
	printf("\n error 31");
	}
	close(fd);	
	
	fd = open("/sys/class/gpio/gpio31/value", O_WRONLY);
	ret=write(fd, "0", sizeof("0"));	
	if(ret<0) {
	printf("\n error in value 31");
	}
	close(fd);
		
	fd = open("/sys/class/gpio/gpio72/value", O_WRONLY);
	ret=write(fd, "0", sizeof("0"));
	if(ret<0) {
	printf("\n error 72");
	}
	close(fd);
	

	fd = open("/sys/class/gpio/gpio24/direction", O_WRONLY);
	ret=write(fd, "out", sizeof("out"));	
	if(ret<0) {
	printf("\n error 24");
	}
	close(fd);	
	
	fd = open("/sys/class/gpio/gpio24/value", O_WRONLY);
	ret=write(fd, "0", sizeof("0"));	
	if(ret<0) {
	printf("\n error in value 24");
	}
	close(fd);


	fd = open("/sys/class/gpio/gpio25/direction", O_WRONLY);
	ret=write(fd, "out", sizeof("out"));	
	if(ret<0) {
	printf("\n error 25");
	}
	close(fd);	
	
	fd = open("/sys/class/gpio/gpio25/value", O_WRONLY);
	ret=write(fd, "0", sizeof("0"));	
	if(ret<0) {
	printf("\n error in value 25");
	}
	close(fd);

	
	fd = open("/sys/class/gpio/gpio44/direction", O_WRONLY);
	ret=write(fd, "out", sizeof("out"));
	if(ret<0) {
	printf("\n error 44");
	}
	close(fd);

	fd = open("/sys/class/gpio/gpio44/value", O_WRONLY);
	ret=write(fd, "1", sizeof("1"));
	if(ret<0) {
	printf("\n error 44");
	}
	close(fd);

	fd = open("/sys/class/gpio/gpio42/direction", O_WRONLY);
	ret=write(fd, "out", sizeof("out"));	
	if(ret<0) {
	printf("\n error 42");
	}
	close(fd);	
	
	fd = open("/sys/class/gpio/gpio42/value", O_WRONLY);
	ret=write(fd, "0", sizeof("0"));	
	if(ret<0) {
	printf("\n error in value 42");
	}
	close(fd);

	fd = open("/sys/class/gpio/gpio43/direction", O_WRONLY);
	ret=write(fd, "out", sizeof("out"));	
	if(ret<0) {
	printf("\n error 43");
	}
	close(fd);	
	
	fd = open("/sys/class/gpio/gpio43/value", O_WRONLY);
	ret=write(fd, "0", sizeof("0"));	
	if(ret<0) {
	printf("\n error in value 43");
	}
	close(fd);

	fd1 = open("/sys/class/gpio/gpio15/direction", O_WRONLY);
	ret=write(fd, "out", sizeof("out"));	
	if(ret<0) {
	printf("\n error 15");
	}
	close(fd);	

	fd1 = open("/sys/class/gpio/gpio15/value", O_WRONLY);
}


int main() {

	gpiosetup();

	pthread_t sensor_thread;
	pthread_create (&sensor_thread, NULL, &distanceFunction, NULL);

	pthread_t display_thread;
	pthread_create (&display_thread, NULL, &displayFunction, NULL);

	

	pthread_join(sensor_thread, NULL);
	pthread_join(display_thread, NULL);
	return 0;
}	
