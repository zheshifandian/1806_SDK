
TARGET             = new_device_listen
TARGET_OBJ         = main.o cJSON.o changes.o utils.o object_save_file.o \
					 local_http.o local_storage.o manager.o iwlib.o iwevent.o \
					 token.o ssst_request.o publish.o device_info.o netdiscover.o \
					 queue.o mtd.o urllist.o sf_factory_read.o

TARGET_CFLAGS      = $(CFLAGS) -I$(STAGING_DIR)/usr/lib/libnet-1.2.x/include
TARGET_LDFLAGS     = $(LDFLAGS) -L$(STAGING_DIR)/usr/lib/libnet-1.2.x/lib

all: $(TARGET)

%.o: %.c
	$(CC) $(TARGET_CFLAGS) -c -o $@ $<

$(TARGET): $(TARGET_OBJ)
	$(CC) $(TARGET_LDFLAGS) -o $@ $(TARGET_OBJ)


clean:
	-rm -f *.elf *.gdb *.o $(TARGET)
