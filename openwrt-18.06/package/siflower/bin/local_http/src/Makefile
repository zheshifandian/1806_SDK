CC= gcc
EXEC = liblocalhttp.so
SRC = local_http.o sf_factory_read.o token.o mtd.o cJSON.o ssst_request.o

LDFLAGS += -shared

all: $(EXEC)
$(EXEC): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(SRC) $(LDLIBS) -lc

clean:
	-rm -f $(EXEC) *.elf *.gdb *.o *.so
