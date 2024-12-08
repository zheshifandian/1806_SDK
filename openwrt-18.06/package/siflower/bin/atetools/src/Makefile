EXEC = ate_cmd
EXEB = ate_server
SRC = main.c
SRB = ate_server.c

all: $(EXEC) $(EXEB)

ate_cmd:$(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LDLIBS)

ate_server:$(SRB)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRB) $(LDLIBS)

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
	-rm -f $(EXEB) *.elf *.gdb *.o
