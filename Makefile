CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -Wshadow -g
TARGET=qr.out
OBJS=main.o bitset.o
LD_LIBS=
LD_FLAGS=-fsanitize=address,undefined -fanalyzer

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LD_FLAGS) $(LD_LIBS) -o $@ $^

%.o: %.c %.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET)

.PHONY: all clean
