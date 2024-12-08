CC = gcc
CFLAGS = -lncurses
TARGET = ink
SRC = ink.c

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS)

clean:
	rm -f $(TARGET)

