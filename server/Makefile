# Compiler
CC = gcc
# Flags
CFLAGS = -Wall -Werror
# Cross compiler variable
CROSS_COMPILE ?=
# Set target
TARGET = aesdsocket
# Set source
SRCS = aesdsocket.c
# Set object
OBJS = aesdsocket.o

# Default build
default: $(TARGET)

all: default

# Cross compile using aarch64-none-linux-gnu-gcc
aarch64-none-linux-gnu-%:
	$(MAKE) CROSS_COMPILE=aarch64-none-linux-gnu-

# Build with appropriate compiler
$(TARGET): $(OBJS)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)
	
# Build object
$(OBJS): $(SRCS)
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c $(SRCS) -o $(OBJS)
	
# Valgrind
valgrind:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=/tmp/valgrind-out.txt ./aesdsocket


# Clean target
clean:
	rm -f $(TARGET) $(OBJS)
