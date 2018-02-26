CC=cc
CFLAGS=-I/usr/include/mariadb -Wall -Werror -O0 -std=c99
LDLIBS=-lwiringPi -lmariadb
OBJECTS=wiegand.o
TARGET=wiegand
RM=rm -f

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDLIBS) -o $@

.PHONY: clean
clean:
	$(RM) $(OBJECTS) $(TARGET)
