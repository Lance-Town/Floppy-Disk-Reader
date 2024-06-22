
# Makefile for compiling a single C file with clang

# Variables
CC = clang
CFLAGS = -Wall -Wextra -pedantic -std=c11
TARGET = main
SRC = main.c

# Default target
all: $(TARGET)

# Compile the C file
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

# Clean target
clean:
	rm -f $(TARGET)

# Phony targets
.PHONY: all clean
