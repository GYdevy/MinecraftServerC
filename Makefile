CC = gcc
CFLAGS = -std=c11 -Wall -IExternals/libnbt
LDFLAGS = -lcurl

SRC = \
    main.c \
    socket_utils.c \
    packet_utils.c \
    handshake.c \
    login.c \
    extract_uuid.c \
    play.c \
    server.c \
    movement.c \
    play_helpers.c

OBJ = $(SRC:.c=.o)

TARGET = server

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
