# Compiler
CC ?= $(CROSS_COMPILE)gcc
# Flags
CFLAGS ?= -g -Wall -Werror
# Cross compiler variable
CROSS_COMPILE ?=
# Set target
TARGET ?= writer
# Set source
SRCS ?= writer.c
# Set object
OBJS ?= writer.o
# Set flags
LDFLAGS ?=

# Default build
default: $(TARGET)

all: default

# Cross compile using aarch64-none-linux-gnu-gcc
aarch64-none-linux-gnu-%:
	$(MAKE) CROSS_COMPILE=aarch64-none-linux-gnu-

# Build with appropriate compiler
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	
# Build object
$(OBJS): $(SRCS)
	$(CC) $(CFLAGS) -c $(SRCS) -o $(OBJS)
	
# Valgrind
valgrind:
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=/tmp/valgrind-out.txt ./aesdsocket


# Clean target
clean:
	rm -f $(TARGET) $(OBJS)
