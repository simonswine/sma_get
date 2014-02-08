
CC = gcc
CFLAGS  = -g -Wall -I/usr/include/libyasdi
TARGET = sma_get
LIBS = -l:libyasdimaster.so.1 
BINDIR = $(DESTDIR)/usr/bin

all: $(TARGET)

$(TARGET).o: $(TARGET).c
	$(CC) $(CFLAGS) -c -o $(TARGET).o $(TARGET).c

$(TARGET): $(TARGET).o
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).o $(LIBS)

clean:
	$(RM) $(TARGET) *.o


install:
	install --mode=755 $(TARGET) $(BINDIR)/
