CFLAGS = -Wall `pkg-config --cflags libmodbus`
INCLUDES := -I../../sysroot/include
LDFLAGS := -L../../sysroot/lib
LIBS += -lpthread `pkg-config --libs libmodbus`

CROSS_COMPILE ?= aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc

SRCS_C := $(wildcard *.c)
OBJS_C := $(SRCS_C:.c=.o)

TARGET := modbus-tcp-server

.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $^

$(TARGET): $(OBJS_C) buildtime.h
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) $(LFLAGS)

install: $(TARGET)
	cp $^ ../../sysroot/bin

buildtime.h: 
	@echo '#ifndef	_BUILDTIME_H_' >  $@
	@echo '#define	_BUILDTIME_H_' >> $@
	@echo '' >> $@
	@date +'#define BUILDTIME "%Y%m%d_%H%M%S"' >> $@
	@echo '#endif' >> $@

all: $(TARGET)

.PHONY: clean

clean:
	rm -f *.o
	rm -f $(TARGET)
	rm -f modbus-tcp-client

client: buildtime.h
	mv -f modbus-tcp-client.c.pc modbus-tcp-client.c
	gcc $(CFLAGS) modbus-tcp-client.c -c -o modbus-tcp-client.o
	gcc modbus-tcp-client.o -o modbus-tcp-client $(LIBS)
	mv -f modbus-tcp-client.c modbus-tcp-client.c.pc
