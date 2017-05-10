CC=gcc
CFLAGS=-c -Wall -Iinclude
LDFLAGS=-static
SOURCES=$(wildcard src/*.c)
OBJECTS=$(SOURCES:.c=.o)
LIBRARY=libese.a

all: $(SOURCES) $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	ar rcs $@ $(OBJECTS)

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(OBJECTS) $(LIBRARY)