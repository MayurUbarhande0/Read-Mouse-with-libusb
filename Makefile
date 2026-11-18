
CC = gcc

CFLAGS = -Wall -Wextra -g
LIBS = $(shell pkg-config --cflags --libs libusb-1.0)

TARGET = main

all: $(TARGET)

$(TARGET): main.o
	$(CC) main.o -o $(TARGET) $(LIBS)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

.PHONY: all clean

clean:
	rm -f *.o $(TARGET)
