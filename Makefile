CC=gcc
CFLAGS=-Wall -Wextra -Wpedantic -Wshadow -g
TARGET=quer.out
OBJS=main.o bitset.o bitstream.o reed_solomon.o
LD_LIBS= -lm -lpng
LD_FLAGS=-fsanitize=address,undefined -fanalyzer

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LD_FLAGS) $(LD_LIBS) -o $@ $^

%.o: %.c %.h
	$(CC) $(LD_FLAGS) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET)

.PHONY: all clean
