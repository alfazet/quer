CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -Wshadow
TARGET=qr.out
OBJS=main.o
LD_LIBS=

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LD_LIBS) $(CFLAGS) -o $@ $^

%.o: %.c %.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET)

.PHONY: all clean
