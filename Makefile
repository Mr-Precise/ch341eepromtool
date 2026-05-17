CC       = gcc
CFLAGS   = -Wall -O2
LDFLAGS  = -lusb-1.0
TARGET   = ch341eeprom
MKTESTIMG_TARGET = mktestimg
SOURCES  = ch341eeprom.c ch341funcs.c
OBJECTS  = $(SOURCES:.c=.o)
PREFIX   = /usr/local
DESTDIR  =

BUILD_MKTESTIMG ?= 0

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c ch341eeprom.h
	$(CC) $(CFLAGS) -c $< -o $@

ifeq ($(BUILD_MKTESTIMG),1)
all: $(MKTESTIMG_TARGET)

$(MKTESTIMG_TARGET): mktestimg.c
	$(CC) $(CFLAGS) -o $@ $^
endif

clean:
	rm -f $(OBJECTS) $(TARGET) $(MKTESTIMG_TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 0755 $(TARGET) $(DESTDIR)$(PREFIX)/bin/

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(TARGET)

.PHONY: all clean install uninstall
