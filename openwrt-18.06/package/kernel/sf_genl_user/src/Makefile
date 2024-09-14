include $(TOPDIR)/.config

GENL_EXEC:=sf_genl_user
GENL_CFLAGS=$(CFLAGS) -I$(STAGING_DIR)/usr/include/ -I$(STAGING_DIR)/usr/include/libnl3
GENL_LDFLAGS = $(LDFLAGS)
GENL_FILES = main.c utils.c genl.c l2.c
ifdef CONFIG_PACKAGE_kmod-dpns_tmu
	GENL_FILES += tmu.c
endif

ifdef CONFIG_PACKAGE_kmod-dpns_mcast
	GENL_FILES += mcast.c
endif

ifdef CONFIG_PACKAGE_kmod-dpns_acl
	GENL_FILES += acl.c
endif

ifndef CONFIG_TARGET_siflower_sf21h8898_mpw0
	GENL_FILES += nat.c vlan.c router.c common.c
endif

GENL_OBJECTS = $(patsubst %.c,%.o,$(GENL_FILES))

$(GENL_EXEC): $(GENL_OBJECTS)
	$(CC) $(GENL_LDFLAGS) -o $(GENL_EXEC) $(GENL_OBJECTS)

%.o: %.c
	$(CC) $(GENL_CFLAGS)  -c -o $@ $<

.PHONY: clean
clean :
	-rm -f *.elf *.gdb *.o $(GENL_EXEC)
