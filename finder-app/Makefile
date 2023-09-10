CC = gcc
CROSS_COMPILE ?=
TARGET = writer

SRCS = writer.c
OBJS = $(SRCS:.c = .o)

default: $(TARGET)

aarch64-none-linux-gnu-%:
	$(MAKE) CROSS_COMPILE = aarch64-none-linux-gnu- $(TARGET)
    
$(TARGET): $(OBJS)
	$(CROSS_COMPILE)$(CC) -o $(TARGET) $(OBJS)
    
clean:
	rm -f $(TARGET)
