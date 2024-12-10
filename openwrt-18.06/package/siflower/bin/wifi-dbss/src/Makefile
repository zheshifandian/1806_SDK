SRC=main.c wifi.c remote_https.c

LIBS = -ljson-c -lubus -lubox -lblobmsg_json

all: wifi-dbss

wifi-dbss: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(INCLUDES) $(LIBS) $(CURL_FLAGS) 

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o
