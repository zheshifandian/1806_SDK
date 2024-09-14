#config

SFAX8_AN8855_SRC_OBJS   := $(an8855_dir)/api/src/air_acl.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_aml.o
#SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_cmd.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_diag.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_error.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_init.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_l2.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_lag.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_led.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_mib.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_mirror.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_port.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_qos.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_sec.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_sptag.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_stp.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_switch.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/api/src/air_vlan.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/core/an8855_init.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/core/an8855_mdio.o
SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/core/an8855_phy_cal.o

SFAX8_AN8855_SRC_OBJS   += $(an8855_dir)/sf_an8855_ops.o
