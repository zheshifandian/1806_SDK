ANDLINK_EXEC	:=	andlink
ANDLINK_CFLAGS	= $(CFLAGS) -D_GNU_SOURCE -DWITH_POSIX -I$(STAGING_DIR)/usr/include/
ANDLINK_LDFLAGS = $(LDFLAGS)
ANDLINK_OBJECTS	= $(patsubst %.c,%.o,$(wildcard *.c))

$(ANDLINK_EXEC): $(ANDLINK_OBJECTS)
	$(CC) $(ANDLINK_LDFLAGS) -o $(ANDLINK_EXEC) $(ANDLINK_OBJECTS)

%.o: %.c
	$(CC) $(ANDLINK_CFLAGS)  -c -o $@ $<

.PHONY: clean

clean :
	-rm -f *.elf *.gdb *.o $(ANDLINK_EXEC)
