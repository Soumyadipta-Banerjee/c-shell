CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2 -Iinclude
TARGET = apex-shell
SRCS = src/main.c src/builtins.c src/execute.c src/parser.c src/signals.c src/jobs.c src/telemetry.c src/fuzzy.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) c-shell my_shell src/*.o *.o

run: all
	./$(TARGET)

test: $(TARGET)
	@bash tests/test_shell.sh

.PHONY: all clean run test
