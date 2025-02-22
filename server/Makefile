CROSS_COMPILE ?=
CC=$(CROSS_COMPILE)gcc
CFLAGS=-Wall -Werror -g
TARGET=aesdsocket
OUTPUT_DIR=.
SRC=$(OUTPUT_DIR)/aesdsocket.c
OUT=$(OUTPUT_DIR)/$(TARGET)

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)

