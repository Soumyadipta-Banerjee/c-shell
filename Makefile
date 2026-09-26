CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2
TARGET = c-shell
SRC = shell.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET) my_shell *.o

run: all
	./$(TARGET)

test: $(TARGET)
	@bash tests/test_shell.sh

.PHONY: all clean run test
