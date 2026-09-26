#!/usr/bin/env bash

# Test suite for c-shell
# Exit with non-zero if any test fails

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
SHELL_BIN="$PROJECT_ROOT/apex-shell"
if [ ! -x "$SHELL_BIN" ]; then
    SHELL_BIN="$PROJECT_ROOT/c-shell"
fi

# Ensure shell binary exists
if [ ! -x "$SHELL_BIN" ]; then
    echo "Error: Shell binary not found or not executable at '$SHELL_BIN'."
    echo "Please run 'make' first."
    exit 1
fi

# Color definitions
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

TOTAL=0
PASSED=0
FAILED=0

# Create temporary sandbox directory for file tests
TEST_DIR="$(mktemp -d -t cshell_test_XXXXXX)"
trap 'rm -rf "$TEST_DIR"' EXIT

run_test() {
    local test_name="$1"
    local input="$2"
    local expected_output="$3"
    local check_type="${4:-exact}" # exact, contains, or exit_only

    TOTAL=$((TOTAL + 1))
    printf "Test %2d: %-50s " "$TOTAL" "$test_name"

    local actual_output
    actual_output=$(printf "%s\n" "$input" | (cd "$TEST_DIR" && "$SHELL_BIN" 2>&1))
    local exit_code=$?

    local pass=0
    case "$check_type" in
        exact)
            if [ "$actual_output" = "$expected_output" ]; then
                pass=1
            fi
            ;;
        contains)
            if echo "$actual_output" | grep -q "$expected_output"; then
                pass=1
            fi
            ;;
        exit_only)
            if [ "$exit_code" -eq 0 ]; then
                pass=1
            fi
            ;;
    esac

    if [ "$pass" -eq 1 ]; then
        PASSED=$((PASSED + 1))
        echo -e "${GREEN}[PASS]${NC}"
    else
        FAILED=$((FAILED + 1))
        echo -e "${RED}[FAIL]${NC}"
        echo "  Input:"
        echo "$input" | sed 's/^/    /'
        echo "  Expected ($check_type):"
        echo "$expected_output" | sed 's/^/    /'
        echo "  Actual:"
        echo "$actual_output" | sed 's/^/    /'
    fi
}

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}      Running Apex Shell Test Suite     ${NC}"
echo -e "${BLUE}========================================${NC}"

# 1. Built-in: pwd
run_test "Built-in: pwd prints working directory" \
    "pwd" \
    "$TEST_DIR" \
    "exact"

# 2. Built-in: cd to directory and pwd
mkdir -p "$TEST_DIR/subdir"
run_test "Built-in: cd to subdirectory and pwd" \
    "cd subdir
pwd" \
    "$TEST_DIR/subdir" \
    "exact"

# 3. Built-in: cd with no arguments defaults to HOME
run_test "Built-in: cd without args defaults to HOME" \
    "cd
pwd" \
    "$HOME" \
    "exact"

# 4. Built-in: cd ~ goes to HOME
run_test "Built-in: cd ~ goes to HOME" \
    "cd ~
pwd" \
    "$HOME" \
    "exact"

# 5. Built-in: cd to non-existent directory reports error
run_test "Built-in: cd to invalid directory reports error" \
    "cd /nonexistent_path_12345" \
    "No such file or directory" \
    "contains"

# 6. Built-in: help lists commands
run_test "Built-in: help displays builtin commands" \
    "help" \
    "cd" \
    "contains"

# 7. Built-in: exit terminates shell execution
run_test "Built-in: exit stops command execution" \
    "echo start
exit
echo unreachable" \
    "start" \
    "exact"

# 8. External: basic command execution
run_test "External: basic command execution (echo)" \
    "echo testing 1 2 3" \
    "testing 1 2 3" \
    "exact"

# 9. Parsing: double quotes preserve spaces
run_test "Parsing: double quotes preserve inner spaces" \
    "echo \"hello   world   from   c-shell\"" \
    "hello   world   from   c-shell" \
    "exact"

# 10. Parsing: single quotes preserve inner spaces
run_test "Parsing: single quotes preserve inner spaces" \
    "echo 'single quoted argument with spaces'" \
    "single quoted argument with spaces" \
    "exact"

# 11. Redirection: Output redirection (>)
run_test "Redirection: output (>) writes file" \
    "echo \"output test\" > out.txt
cat out.txt" \
    "output test" \
    "exact"

# 12. Redirection: Output append redirection (>>)
run_test "Redirection: append (>>) adds content" \
    "echo \"first line\" > append.txt
echo \"second line\" >> append.txt
cat append.txt" \
    "first line
second line" \
    "exact"

# 13. Redirection: Input redirection (<)
echo "input file content" > "$TEST_DIR/input.txt"
run_test "Redirection: input (<) reads file" \
    "cat < input.txt" \
    "input file content" \
    "exact"

# 14. Redirection: Combined input and output redirection
echo "data to copy" > "$TEST_DIR/source.txt"
run_test "Redirection: combined input (<) and output (>)" \
    "cat < source.txt > dest.txt
cat dest.txt" \
    "data to copy" \
    "exact"

# 15. Pipeline: 2-stage pipeline
run_test "Pipeline: 2-stage pipeline (echo | tr)" \
    "echo \"pipeline test\" | tr \"a-z\" \"A-Z\"" \
    "PIPELINE TEST" \
    "exact"

# 16. Pipeline: 3-stage pipeline
run_test "Pipeline: 3-stage pipeline (echo | tr | wc -l)" \
    "echo \"one two three four\" | tr \" \" \"\n\" | wc -l" \
    "4" \
    "exact"

# 17. Pipeline: filtering with grep
run_test "Pipeline: filtering stream with grep" \
    "printf \"apple\nbanana\ncherry\n\" | grep banana" \
    "banana" \
    "exact"

# 18. Redirection: missing file syntax error
run_test "Redirection: syntax error when target missing" \
    "echo test >" \
    "syntax error near unexpected token 'newline'" \
    "contains"

# 19. Pipeline: empty pipe stage syntax error
run_test "Pipeline: syntax error on invalid empty pipe" \
    "ls | | grep" \
    "syntax error near unexpected token '|'" \
    "contains"

# 20. Empty line / whitespace input handled gracefully
run_test "Robustness: blank lines and whitespace do not crash" \
    "   

echo alive" \
    "alive" \
    "exact"

# 21. Signal Handling: SIGINT directly to shell does not kill it
TOTAL=$((TOTAL + 1))
printf "Test %2d: %-50s " "$TOTAL" "Signals: SIGINT does not kill shell at prompt"
python3 -c "
import subprocess, time, signal, sys
p = subprocess.Popen(['$SHELL_BIN'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
time.sleep(0.1)
p.send_signal(signal.SIGINT)
time.sleep(0.1)
stdout, _ = p.communicate(input='echo alive\nexit\n', timeout=2)
if 'alive' in stdout:
    sys.exit(0)
sys.exit(1)
" > /dev/null 2>&1
if [ $? -eq 0 ]; then
    PASSED=$((PASSED + 1))
    echo -e "${GREEN}[PASS]${NC}"
else
    FAILED=$((FAILED + 1))
    echo -e "${RED}[FAIL]${NC}"
fi

# 22. Signal Handling: SIGINT terminates running child while shell survives
TOTAL=$((TOTAL + 1))
printf "Test %2d: %-50s " "$TOTAL" "Signals: SIGINT interrupts child, shell survives"
python3 -c "
import subprocess, time, signal, os, sys
p = subprocess.Popen(['$SHELL_BIN'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
p.stdin.write('sleep 5\n')
p.stdin.flush()
time.sleep(0.3)
ps = subprocess.run(['pgrep', '-P', str(p.pid), 'sleep'], capture_output=True, text=True)
child_pid = ps.stdout.strip()
if not child_pid:
    sys.exit(1)
os.kill(int(child_pid), signal.SIGINT)
time.sleep(0.2)
p.stdin.write('echo child_reaped_shell_running\nexit\n')
p.stdin.flush()
stdout, _ = p.communicate(timeout=3)
if 'child_reaped_shell_running' in stdout:
    sys.exit(0)
sys.exit(1)
" > /dev/null 2>&1
if [ $? -eq 0 ]; then
    PASSED=$((PASSED + 1))
    echo -e "${GREEN}[PASS]${NC}"
else
    FAILED=$((FAILED + 1))
    echo -e "${RED}[FAIL]${NC}"
fi

# 23. Chaining: Semicolon (;) sequential execution
run_test "Chaining: semicolon (;) executes in sequence" \
    "echo first; echo second; echo third" \
    "first
second
third" \
    "exact"

# 24. Chaining: Logical AND (&&) executes on success
run_test "Chaining: AND (&&) executes next on success" \
    "true && echo and_ok" \
    "and_ok" \
    "exact"

# 25. Chaining: Logical AND (&&) skips on failure
run_test "Chaining: AND (&&) skips next on failure" \
    "false && echo unreachable || echo reached" \
    "reached" \
    "exact"

# 26. Chaining: Logical OR (||) executes on failure
run_test "Chaining: OR (||) executes on failure" \
    "false || echo fallback" \
    "fallback" \
    "exact"

# 27. Chaining: Logical OR (||) skips on success
run_test "Chaining: OR (||) skips next on success" \
    "true || echo unreachable
echo after" \
    "after" \
    "exact"

# 28. Expansion: $? reflects previous command exit code
run_test "Expansion: \$? reflects previous command status" \
    "false ; echo \$? ; true ; echo \$?" \
    "1
0" \
    "exact"

# 29. Expansion: $VAR expands and single quotes preserve literal
run_test "Expansion: \$VAR expands and single quotes preserve" \
    "echo \$USER
echo '\$USER'" \
    "$USER
\$USER" \
    "exact"

# 30. Built-in: exit [code] sets shell exit code
TOTAL=$((TOTAL + 1))
printf "Test %2d: %-50s " "$TOTAL" "Built-in: exit with custom status code (exit 37)"
printf "exit 37\n" | "$SHELL_BIN" > /dev/null 2>&1
EXIT_VAL=$?
if [ "$EXIT_VAL" -eq 37 ]; then
    PASSED=$((PASSED + 1))
    echo -e "${GREEN}[PASS]${NC}"
else
    FAILED=$((FAILED + 1))
    echo -e "${RED}[FAIL]${NC} (got $EXIT_VAL, expected 37)"
fi

# 31. Background: job launches asynchronously with &
run_test "Background: job launches asynchronously with &" \
    "sleep 1 &" \
    "[1]" \
    "contains"

# 32. Background: jobs builtin lists active tasks
run_test "Background: jobs builtin lists active tasks" \
    "sleep 2 &
jobs" \
    "Running" \
    "contains"

# 33. Background: finished job is reaped and reported
run_test "Background: finished job is reaped and reported" \
    "sleep 0.4 &
sleep 0.7
jobs" \
    "Done" \
    "contains"

# 34. Built-in: export sets environment variable
run_test "Built-in: export and variable expansion" \
    "export APEX_TEST_VAR=ApexRocks
echo \$APEX_TEST_VAR" \
    "ApexRocks" \
    "exact"

# 35. Built-in: unset removes environment variable
run_test "Built-in: unset removes variable" \
    "export APEX_TEMP_VAR=Temporary
unset APEX_TEMP_VAR
echo [\$APEX_TEMP_VAR]" \
    "[]" \
    "exact"

# 36. Built-in: env displays environment variables
run_test "Built-in: env displays environment entries" \
    "export APEX_ENV_TEST=Discovered
env" \
    "APEX_ENV_TEST=Discovered" \
    "contains"

# 37. Expansion: tilde (~) expands to HOME
run_test "Expansion: tilde (~) expands to HOME" \
    "echo ~" \
    "$HOME" \
    "exact"

# 38. Expansion: braced variables ${VAR} inside text
run_test "Expansion: braced \${VAR} inside text" \
    "export APEX_PREFIX=Super
echo \${APEX_PREFIX}_Shell" \
    "Super_Shell" \
    "exact"

echo -e "${BLUE}========================================${NC}"
if [ "$FAILED" -eq 0 ]; then
    echo -e "${GREEN}All $TOTAL tests passed successfully!${NC}"
    exit 0
else
    echo -e "${RED}Test Results: $PASSED passed, $FAILED failed out of $TOTAL.${NC}"
    exit 1
fi

