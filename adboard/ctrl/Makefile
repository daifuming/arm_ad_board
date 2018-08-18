CC=gcc
TARGET=ctrl
#SRCS=$(wildcard *.c)
#OBJS=$(patsubst %.c, %.o, $(SRCS))
OBJS=$(patsubst %.c, %.o, $(wildcard *.c))
$(TARGET):$(OBJS)
	$(CC) -o $(TARGET) $(OBJS)

clean:
	rm $(OBJS) $(TARGET)