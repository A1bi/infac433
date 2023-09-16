CFLAGS = -llgpio -lpthread -Wall -IMQTT-C/include
SOURCE = $(wildcard *.c) $(wildcard MQTT-C/src/*.c)
OBJECTS = $(SOURCE:.c=.o)
BUILD_DIR = build
BIN_NAME = infac433

ifeq ($(BUILD),debug)
	CFLAGS += -g
else
	CFLAGS += -O3
endif

$(BUILD_DIR)/$(BIN_NAME): $(addprefix build/,$(OBJECTS))
	$(CC) $^ $(CFLAGS) -o $@

$(BUILD_DIR)/%.o: %.c
	mkdir -p $(dir $@)
	$(CC) -c $^ $(CFLAGS) -o $@

clean:
	rm -Rf build
