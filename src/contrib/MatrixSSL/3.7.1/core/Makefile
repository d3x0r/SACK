#
#	Makefile for core dynamic and static library
#
#	Copyright (c) INSIDE Secure, 2013. All Rights Reserved.
#

include ../common.mk

LIBRARY	= libcore$(SO)
STATIC	= libcorestatic$(A)

OBJECTS	= \
		corelib$(O) \
		$(OSDEP)/osdep$(O) 

#
#	Compile options
#
CFLAGS  += -I./ -I../

all: compile

compile: $(OBJECTS) $(STATIC) $(LIBRARY)

*.o: *.h *.c */*.c

#
# Build the library
#
$(LIBRARY): $(OBJECTS)
	$(CC) $(SHARED) -o $@ $^ $(LDFLAGS)
	$(STRIP) $(LIBRARY)

#
# Build the static library
# Direct stderr to null so we don't see the 'empty file' warnings
#
$(STATIC): $(OBJECTS)
	$(AR) -rcuv $@ $^ 2>/dev/null

#
#	Clean up all generated files
#
clean:
	rm -f $(LIBRARY) $(STATIC) $(OBJECTS)
