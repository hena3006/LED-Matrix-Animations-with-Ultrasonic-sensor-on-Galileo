# KDIR:=/opt/iot-devkit/1.7.2/sysroots/i586-poky-linux/usr/src/kernel
# PWD:= $(shell pwd)
# PATH=${PATH}:/opt/iot-devkit/1.7.2/sysroots/x86_64-pokysdk-linux/usr/bin/i586-poky-linux

IOT_HOME = /opt/iot-devkit/1.7.2/sysroots/i586-poky-linux/

PATH := $(PATH):$(IOT_HOME)/x86_64-pokysdk-linux/usr/bin/i586-poky-linux

CC = i586-poky-linux-gcc
ARCH = x86
CROSS_COMPILE = /opt/iot-devkit/1.7.2/sysroots/x86_64-pokysdk-linux/usr/bin/i586-poky-linux/i586-poky-linux-
SROOT=$(IOT_HOME)

APP1 = sensor


all:
	
	$(CROSS_COMPILE)gcc -Wall -o $(APP1)  animation.c -pthread


clean:
	rm -f *.o
	rm -f $(APP1) 
	
