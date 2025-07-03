CC=gcc
FLAGS=-Wall -Wextra -Wpedantic -Wshadow -O3
TARGET=quer.out
OBJS=main.o bitset.o bitstream.o reed_solomon.o
LD_LIBS= -lm -lpng

PREFIX ?= /usr/bin

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LD_LIBS) -o $@ $^

%.o: %.c %.h
	$(CC) $(FLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET)

install: all
	install -D quer.out $(DESTDIR)$(PREFIX)/quer

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/quer

.PHONY: all clean install uninstall
