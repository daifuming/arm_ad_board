CC=arm-linux-gcc
TARGET=client
#SRCS=$(wildcard *.c)
#OBJS=$(patsubst %.c, %.o, $(SRCS))
OBJS=$(patsubst %.c, %.o, $(wildcard *.c))
$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(OBJS) -pthread -lm

clean:
	rm $(OBJS) $(TARGET)