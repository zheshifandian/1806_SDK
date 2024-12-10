CC = gcc
#CFLAGS = -Wall -Wno-unused-variable
#CFLAGS = -Wno-unused
OBJS = cloud_common.o local_stub.o subcloud.o local_storage.o queue.o remote_https.o server_intf.o token.o mtd.o check_bindid.c cJSON.o

LIBS = -luci -lcurl -lwebsockets -lcrypto -lc -llocalhttp

SICFLAGS = -Wno-unused-variable

all: subcloud

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

subcloud: $(OBJS)
	$(CC) -o $@ $(SICFLAGS) $(OBJS) $(LIBS)

clean:
	rm -f rbcfg *.o
