
SOCK_SERVER             = ssst
SOCK_SERVER_LIB_OBJ     = main.o bind.o changes.o status_sync.o utils.o object_save_file.o \
                          local_storage.o manager.o iwlib.o iwevent.o\
						publish.o device_info.o netdiscover.o queue.o urllist.o ker_health.o

SOCK_SERVER_CFLAGS      = $(CFLAGS) -I$(STAGING_DIR)/usr/lib/libnet-1.2.x/include

SOCK_SERVER_LDFLAGS     = $(LDFLAGS) -L$(STAGING_DIR)/usr/lib/libnet-1.2.x/lib

P2P_CCFLAGS     = $(CCFLAGS) -g
P2P_LDFLAGS     = $(LDFLAGS)

#MY_FLAG	= -DZIGBEE_SUPPORT -DKEVIN_TEST

$(SOCK_SERVER): $(SOCK_SERVER_LIB_OBJ)
	$(CXX) $(SOCK_SERVER_LDFLAGS) -o $(SOCK_SERVER) $(SOCK_SERVER_LIB_OBJ)

%.o: %.c
	$(CC) $(SOCK_SERVER_CFLAGS) $(MY_FLAG) -c -o $@ $<

%.o: %.cpp
	$(CXX) $(P2P_CCFLAGS) -c -o $@ $<

#compile: clean $(SOCK_SERVER_LIB_OBJ)
#	$(CC) $(SOCK_SERVER_LDFLAGS) $(MY_FLAG) -o $(SOCK_SERVER) $(SOCK_SERVER_LIB_OBJ)

.PHONY: clean

clean:
	-rm -f *.elf *.gdb *.o $(SOCK_SERVER)
