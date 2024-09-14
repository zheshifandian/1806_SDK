# This is a Reference file and can be modified by customer
# depends on customer build environment.
# Makefile for Switch API
#

#TARGET:= $(XX)

# Target module name (Build as a Module)
TARGET := swapi-mod
#EXTRA_CFLAGS += -I$(KERNEL_SRC)$SourcePath
#Extra symbol versions (External symbols used by this module)
#export KBUILD_EXTRA_SYMBOLS=$(PKG_CONFIG

# Target objects
$(TARGET)-y += gsw_io_wrap.o \
	gsw_sw_init.o \
	gsw_api_func.o \
	gsw_core.o \
	gsw_pce.o \
	gsw_vlan.o \
	gsw_qos.o \
	gsw_multi.o

# Build as part of Kernel driver
obj-$(XX)	+= drv_switch_api.o

ifeq ($(XX),y)
	drv_switch_api-objs	:= gsw_io_wrap.o
	drv_switch_api-objs	+= gsw_sw_init.o
	drv_switch_api-objs   += gsw_api_func.o
 	drv_switch_api-objs   += gsw_core.o
 	drv_switch_api-objs   += gsw_pce.o
 	drv_switch_api-objs   += gsw_vlan.o
 	drv_switch_api-objs   += gsw_qos.o
 	drv_switch_api-objs   += gsw_multi.o
endif
