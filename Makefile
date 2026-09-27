CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -O2 -Iinclude
TARGET = apex-shell
SRCS = src/main.c src/builtins.c src/execute.c src/parser.c src/signals.c src/jobs.c src/telemetry.c src/fuzzy.c src/history.c src/alias.c src/safety.c src/linereader.c
OBJS = $(SRCS:.c=.o)

UNIT_TESTS = tests/unit/test_fuzzy tests/unit/test_alias

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# C Unit Tests
tests/unit/test_fuzzy: tests/unit/test_fuzzy.c src/fuzzy.c
	$(CC) $(CFLAGS) $^ -o $@

tests/unit/test_alias: tests/unit/test_alias.c src/alias.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(TARGET) c-shell my_shell src/*.o *.o $(UNIT_TESTS)

run: all
	./$(TARGET)

test: $(TARGET)
	@bash tests/run_tests.sh

test-fast: $(TARGET)
	@bash tests/run_tests.sh --fast

test-unit: $(UNIT_TESTS)
	@./tests/unit/test_fuzzy
	@./tests/unit/test_alias

test-all: $(TARGET) $(UNIT_TESTS)
	@bash tests/run_tests.sh --all

test-suite: $(TARGET)
	@bash tests/run_tests.sh $(SUITE)

.PHONY: all clean run test test-fast test-unit test-all test-suite
