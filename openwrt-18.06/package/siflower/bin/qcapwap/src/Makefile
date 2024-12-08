COMMON_OBJS = CWLog.o tlv.o network.o capwap_message.o capwap_common.o
AC_OBJS = AC.o ac_mainloop.o ac_discovery.o ac_interface.o uci_config.o ac_manager.o ac_ieee80211.o rbtree.o ac_message.o
WTP_OBJS = WTP.o wpa_ctrl.o wtp_hostapd.o os_unix.o config_file.o wtp_compat_message.o

WUM_OBJS = WUM.o

AC_NAME = AC
WUM_NAME = WUM
WTP_NAME = WTP

CFLAGS += -Wall -std=gnu99
LDFLAGS += -levent_core -lpthread -D_REENTRANT -luci -ljson-c

all: $(AC_NAME) $(WUM_NAME) $(WTP_NAME)

$(AC_NAME): $(AC_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(AC_OBJS) $(COMMON_OBJS) -o $(AC_NAME)

$(WUM_NAME): $(WUM_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(WUM_OBJS) -o $(WUM_NAME)

$(WTP_NAME): $(WTP_OBJS) $(COMMON_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(WTP_OBJS) $(COMMON_OBJS) -o $(WTP_NAME)
