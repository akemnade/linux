#KSRC := /lib/modules/$(KVER)/build
#all: modules

#modules:
#	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KSRC) M=$(shell pwd)  modules

#
# Makefile for the touch pad drivers.
#

# Each configuration option enables a list of files.

# 
#CONFIG_TOUCHPAD_ELAN_SPI_EKT1059 := m
elan_tp_spi-y := elan_tp_arrowkey.o \
	elan_tp_touch.o \
	elan_tp_mouse.o \
	elan_tp_util.o \
	elan_tp_tap.o \
	elan_tp_alg.o \
	elan_ekt1059_spi.o

obj-$(CONFIG_TOUCHPAD_ELAN_SPI_EKT1059) := elan_tp_spi.o
