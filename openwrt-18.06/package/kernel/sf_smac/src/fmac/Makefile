SUBDIRS ?= drivers/net/wireless/siflower
umac_dir=$(SUBDIRS)/fmac/bb_src/umac
lmac_dir=$(SUBDIRS)/fmac/bb_src/lmac
export umac_dir
export lmac_dir

obj-$(CONFIG_SF16A18_WIFI_FULL_MAC) += sf16a18_fmac.o


root_src_ref_dir = ./bb_src/umac
export root_src_ref_dir

include $(umac_dir)/fullmac/fullmac.mk

ccflags-y += -I$(SUBDIRS)/rf/

sf16a18_fmac-y += $(SF16A18_FULLMAC_SRC_OBJS)
