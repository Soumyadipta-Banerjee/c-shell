CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2 -Iinclude
TARGET = c-shell
SRCS = src/main.c src/builtins.c src/execute.c src/parser.c src/signals.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) my_shell src/*.o *.o

run: all
	./$(TARGET)

test: $(TARGET)
	@bash tests/test_shell.sh

.PHONY: all clean run test
