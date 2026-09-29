CC ?= gcc
CFLAGS ?= -Wall -Wextra -pedantic -std=c99 -O2 -Iinclude
TARGET = apex-shell
SRCS = src/main.c src/builtins.c src/execute.c src/parser.c src/expander.c src/prompt.c src/redirection.c src/globber.c src/signals.c src/jobs.c src/telemetry.c src/fuzzy.c src/history.c src/alias.c src/safety.c src/linereader.c src/highlight.c src/completion.c src/arithmetic.c
OBJS = $(SRCS:.c=.o)
NON_MAIN_OBJS = $(filter-out src/main.o, $(OBJS))

UNIT_TESTS = tests/unit/test_fuzzy tests/unit/test_alias tests/unit/test_arithmetic tests/unit/test_glob tests/unit/test_prompt

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

tests/unit/test_arithmetic: tests/unit/test_arithmetic.c src/arithmetic.c
	$(CC) $(CFLAGS) $^ -o $@

tests/unit/test_glob: tests/unit/test_glob.c $(NON_MAIN_OBJS)
	$(CC) $(CFLAGS) $^ -o $@

tests/unit/test_prompt: tests/unit/test_prompt.c src/prompt.c
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
	@./tests/unit/test_arithmetic
	@./tests/unit/test_glob
	@./tests/unit/test_prompt

test-all: $(TARGET) $(UNIT_TESTS)
	@bash tests/run_tests.sh --all

test-suite: $(TARGET)
	@bash tests/run_tests.sh $(SUITE)

asan:
	$(MAKE) clean
	$(MAKE) all $(UNIT_TESTS) CFLAGS="-Wall -Wextra -pedantic -std=c99 -O1 -g -fsanitize=address,undefined -Iinclude"

test-asan: asan
	@bash tests/run_tests.sh --all

.PHONY: all clean run test test-fast test-unit test-all test-suite asan test-asan
