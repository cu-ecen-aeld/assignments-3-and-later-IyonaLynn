CROSS_COMPILE ?=
CC ?= $(CROSS_COMPILE)gcc
CFLAGS ?= -Wall -Werror -g -pthread
LDFLAGS ?= -lrt

TARGET=aesdsocket
SRC=$(TARGET).c
OUT=$(TARGET)

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -D_POSIX_C_SOURCE=200112L -o $(OUT) $(SRC) $(LDFLAGS)

clean:
	rm -f $(OUT) *.o
