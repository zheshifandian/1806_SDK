TEMP_CTRL_PROTECT             = tcontrol
TEMP_CTRL_PROTECT_OBJ         = tcontrol.o
TEMP_CTRL_PROTECT_CFLAGS      = $(CFLAGS) -Wall

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

compile: clean $(TEMP_CTRL_PROTECT_OBJ)
	$(CC) $(LDFLAGS) -o $(TEMP_CTRL_PROTECT) $(TEMP_CTRL_PROTECT_OBJ)

clean:
	-rm -f *.elf *.gdb *.o $(TEMP_CTRL_PROTECT)
