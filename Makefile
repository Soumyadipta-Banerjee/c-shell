CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = c-shell
SRC = shell.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) *.o

run: all
	./$(TARGET)

.PHONY: all clean run
