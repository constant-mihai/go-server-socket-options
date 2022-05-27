CC=gcc
CFLAGS=-I. -lpthread -Wall -Wextra -fno-diagnostics-show-caret -Wcast-align -Werror=return-type -g -O0 -Ddebug #-fPIC
DEPS = udp.h
OBJ = udp.o main.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

udp: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf *.o
	rm udp
