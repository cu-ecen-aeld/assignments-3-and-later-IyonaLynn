CROSS_COMPILE ?= aarch64-buildroot-linux-gnu-
CC=$(CROSS_COMPILE)gccCFLAGS=-Wall -Werror -g
TARGET=aesdsocket
OUTPUT_DIR=.
SRC=$(OUTPUT_DIR)/aesdsocket.c
OUT=$(OUTPUT_DIR)/$(TARGET)

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(OUT) $(SRC)

clean:
	rm -f $(OUT)
