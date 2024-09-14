#config

INC_DIR := $(intel_dir)/include
SRC_DIR := $(intel_dir)/src

# intel7084 api build
ifdef CONFIG_SFAX8_ESWITCH_LIGHT
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/sf_intel7084_api.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/sf_intel7084_ops.o
else
SFAX8_INTEL7084_SRC_OBJS   := $(SRC_DIR)/gsw_io_wrap.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/gsw_sw_init.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/gsw_api_func.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/gsw_core.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/gsw_pce.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/gsw_vlan.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/gsw_qos.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/gsw_multi.o
SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/sf_intel7084_ops.o
#SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/sf_intel7084_qos_api.o
endif

SFAX8_INTEL7084_SRC_OBJS   += $(SRC_DIR)/sf_intel7084_bridge_redirect.o
ccflags-y += -I$(INC_DIR)
