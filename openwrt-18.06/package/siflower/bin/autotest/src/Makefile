EXEC=autotest
EXEB=random_name
EXED=flash_speed_test
EXEA=autotest_init

SRC=socket_server.c
SRB=random_name.c
SRD=flash_speed_test.c
SRA=autotest_init.c silogserver.c

all: $(EXEC)  $(EXEB) $(EXED) $(EXEA)
autotest: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LDLIBS)

random_name: $(SRB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRB) $(LDLIBS)

flash_speed_test: $(SRD)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRD) $(LDLIBS)

autotest_init: $(SRA)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRA) $(LDLIBS)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
	-rm -f $(EXEB) *.elf *.gdb *.o
	-rm -f $(EXED) *.elf *.gdb *.o
	-rm -f $(EXEA) *.elf *.gdb *.o
