
PING_PACKAGE_LOST             = tscfg
PING_PACKAGE_LOST_LIB_OBJ     = tsconfig.o tstable.o cJSON.o llist.o hash.o
PING_PACKAGE_LOST_CFLAGS      = $(CFLAGS) -Wunused-but-set-variable

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

compile: clean $(PING_PACKAGE_LOST_LIB_OBJ)
	$(CC) $(LDFLAGS) -o $(PING_PACKAGE_LOST) $(PING_PACKAGE_LOST_LIB_OBJ)

clean:
	-rm -f *.elf *.gdb *.o $(PING_PACKAGE_LOST)
