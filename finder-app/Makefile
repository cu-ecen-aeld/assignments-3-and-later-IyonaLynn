# Makefile

# Cross-compilation support
CROSS_COMPILE ?=
CC = $(CROSS_COMPILE)gcc  # Ensure 'gcc' is used explicitly
CFLAGS = -Wall -Werror -g -O2       # Compiler flags for warnings and debugging

# List of object files
OBJ = writer.o

# Default target to build the writer application
#Added in A4 because build.sh was expecting all target
all: writer
writer: $(OBJ)
	$(CC) -o writer $(OBJ)

# Compile writer.o from writer.c
writer.o: writer.c
	$(CC) $(CFLAGS) -c writer.c

# Clean target to remove the application and all .o files
clean:
	rm -f writer $(OBJ)
