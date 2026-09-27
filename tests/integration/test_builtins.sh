#!/usr/bin/env bash
# ==============================================================================
# Suite: Built-in Commands & Directory Navigation (tests/integration/test_builtins.sh)
# ==============================================================================

set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../helpers/test_framework.sh"
setup_test_env

echo -e "${BLUE}▶ Running Suite: Built-in Commands & Navigation${NC}"

# 1. pwd
run_test "pwd prints working directory" \
    "pwd" \
    "$TEST_DIR" \
    "exact"

# 2. cd to subdirectory
mkdir -p "$TEST_DIR/subdir"
run_test "cd to subdirectory and pwd" \
    "cd subdir
pwd" \
    "$TEST_DIR/subdir" \
    "exact"

# 3. cd without args
run_test "cd without args defaults to HOME" \
    "cd
pwd" \
    "$HOME" \
    "exact"

# 4. cd ~
run_test "cd ~ goes to HOME" \
    "cd ~
pwd" \
    "$HOME" \
    "exact"

# 5. cd to invalid path
run_test "cd to invalid directory reports error" \
    "cd /nonexistent_path_12345" \
    "No such file or directory" \
    "contains"

# 6. help
run_test "help displays builtin commands" \
    "help" \
    "cd" \
    "contains"

# 7. exit terminates
run_test "exit stops command execution" \
    "echo start
exit
echo unreachable" \
    "start" \
    "exact"

# 8. exit custom code
SUITE_TOTAL=$((SUITE_TOTAL + 1))
printf "  Test %2d: %-52s " "$SUITE_TOTAL" "exit with custom status code (exit 37)"
printf "exit 37\n" | (cd "$TEST_DIR" && "$SHELL_BIN") > /dev/null 2>&1
EXIT_VAL=$?
if [ "$EXIT_VAL" -eq 37 ]; then
    SUITE_PASSED=$((SUITE_PASSED + 1))
    echo -e "${GREEN}[PASS]${NC}"
else
    SUITE_FAILED=$((SUITE_FAILED + 1))
    echo -e "${RED}[FAIL]${NC} (got $EXIT_VAL, expected 37)"
fi

# 9. export
run_test "export and variable expansion" \
    "export APEX_TEST_VAR=ApexRocks
echo \$APEX_TEST_VAR" \
    "ApexRocks" \
    "exact"

# 10. unset
run_test "unset removes variable" \
    "export APEX_TEMP_VAR=Temporary
unset APEX_TEMP_VAR
echo [\$APEX_TEMP_VAR]" \
    "[]" \
    "exact"

# 11. env
run_test "env displays environment entries" \
    "export APEX_ENV_TEST=Discovered
env" \
    "APEX_ENV_TEST=Discovered" \
    "contains"

# 12. dirs
run_test "dirs displays working directory" \
    "dirs" \
    "$TEST_DIR" \
    "contains"

# 13. pushd & popd
mkdir -p "$TEST_DIR/sub1"
run_test "pushd and popd traverse stack" \
    "pushd sub1
pwd
popd
pwd" \
    "$TEST_DIR/sub1
$TEST_DIR" \
    "contains"

# 14. popd empty
run_test "popd on empty stack reports error" \
    "popd" \
    "directory stack empty" \
    "contains"

# 15. z jump
mkdir -p "$TEST_DIR/my_special_dir"
run_test "z jumps to visited directory" \
    "cd my_special_dir
cd ..
z special
pwd" \
    "my_special_dir" \
    "contains"

suite_summary "Built-ins & Navigation"
exit $?
