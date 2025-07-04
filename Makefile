.POSIX:
.SUFFIXES:	.c .o
.PHONY:		all clean install

# Build configuration, override on make invocation
CC=		cc
CFLAGS=		-Wall -Wextra -pedantic -O2
INCLUDES=	-I/usr/local/include
LDFLAGS=	-L/usr/local/lib
PREFIX=		/usr/local

# Project configuration, do not override
TARGET=		quer.out
OBJS=		main.o bitset.o bitstream.o reed_solomon.o
CSTD=		c23
LIBS=		-lpng

# Default target
all:		$(TARGET)

# Header depedencies
bitset.o:	bitset.h
bitstream.o:	bitstream.h
main.o:		bitset.h bitstream.h reed_solomon.h
reed_solomon.o:	reed_solomon.h

$(TARGET): $(OBJS)
	$(CC) -o $@ -std=$(CSTD) $(CFLAGS) $(LDFLAGS) $(OBJS) $(LIBS)

.c.o:
	$(CC) -c -o $@ -std=$(CSTD) $(CFLAGS) $(INCLUDES) $<

clean:
	rm -f $(OBJS)

install: all
	install -d -m755 $(DESTDIR)$(PREFIX)/bin
	install -s -m755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/quer

