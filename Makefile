
CC = gcc
CFLAGS  = -g -Wall -I/usr/include/libyasdi
TARGET = sma_get
LIBS = -l:libyasdimaster.so.1 

all: $(TARGET)

$(TARGET).o: $(TARGET).c
	$(CC) $(CFLAGS) -c -o $(TARGET).o $(TARGET).c

$(TARGET): $(TARGET).o
	$(CC) $(LIBS) $(CFLAGS) -o $(TARGET) $(TARGET).o

clean:
	$(RM) $(TARGET) *.o
