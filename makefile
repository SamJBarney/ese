CC=gcc
CFLAGS=-c -Wall -Iinclude
LDFLAGS=-shared
SOURCES=$(wildcard src/*.c)
OBJECTS=$(SOURCES:.c=.o)
LIBRARY=libese.so

all: $(SOURCES) $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(LIBRARY)