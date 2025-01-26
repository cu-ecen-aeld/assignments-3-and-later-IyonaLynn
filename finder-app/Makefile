# Makefile
# Cross-compilation support
CC ?= gcc            # Default to gcc if CC is not defined
CROSS_COMPILE ?=     # Empty by default, specify when cross-compiling
CFLAGS = -Wall       # Compiler flags

# List of object files
OBJ = writer.o

# Default target to build the writer application
writer: $(OBJ)
	$(CROSS_COMPILE)$(CC) -o writer $(OBJ)

# Compile writer.o from writer.c
writer.o: writer.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c writer.c

# Clean target to remove the application and all .o files
clean:
	rm -f writer $(OBJ)
