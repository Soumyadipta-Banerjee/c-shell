#!/usr/bin/env bash
# ==============================================================================
# Suite: Scripting, History & Sourcing (tests/integration/test_scripting.sh)
# ==============================================================================

set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../helpers/test_framework.sh"
setup_test_env

echo -e "${BLUE}▶ Running Suite: Scripting, Sourcing & History${NC}"

# 1. Basic external command
run_test "basic command execution (echo)" \
    "echo testing 1 2 3" \
    "testing 1 2 3" \
    "exact"

# 2. Blank lines and whitespace robustness
run_test "blank lines and whitespace do not crash" \
    "   

echo alive" \
    "alive" \
    "exact"

# 3. Inline -c execution
SUITE_TOTAL=$((SUITE_TOTAL + 1))
printf "  Test %2d: %-52s " "$SUITE_TOTAL" "-c runs inline command string"
OUTPUT=$("$SHELL_BIN" -c "echo HelloInline")
if [ "$OUTPUT" = "HelloInline" ]; then
    SUITE_PASSED=$((SUITE_PASSED + 1))
    echo -e "${GREEN}[PASS]${NC}"
else
    SUITE_FAILED=$((SUITE_FAILED + 1))
    echo -e "${RED}[FAIL]${NC} (got '$OUTPUT', expected 'HelloInline')"
fi

# 4. Inline -c exit code
SUITE_TOTAL=$((SUITE_TOTAL + 1))
printf "  Test %2d: %-52s " "$SUITE_TOTAL" "-c preserves exit status code"
"$SHELL_BIN" -c "exit 42" > /dev/null 2>&1
EXIT_VAL=$?
if [ "$EXIT_VAL" -eq 42 ]; then
    SUITE_PASSED=$((SUITE_PASSED + 1))
    echo -e "${GREEN}[PASS]${NC}"
else
    SUITE_FAILED=$((SUITE_FAILED + 1))
    echo -e "${RED}[FAIL]${NC} (got $EXIT_VAL, expected 42)"
fi

# 5. File execution ignores # comments
SUITE_TOTAL=$((SUITE_TOTAL + 1))
printf "  Test %2d: %-52s " "$SUITE_TOTAL" "file execution ignores # comments"
printf "# Comment\necho Line1\n# Another comment\necho Line2\n" > "$TEST_DIR/test_run.apex"
OUTPUT=$("$SHELL_BIN" "$TEST_DIR/test_run.apex")
EXPECTED="Line1
Line2"
if [ "$OUTPUT" = "$EXPECTED" ]; then
    SUITE_PASSED=$((SUITE_PASSED + 1))
    echo -e "${GREEN}[PASS]${NC}"
else
    SUITE_FAILED=$((SUITE_FAILED + 1))
    echo -e "${RED}[FAIL]${NC}"
fi

# 6. File execution preserves exit code
SUITE_TOTAL=$((SUITE_TOTAL + 1))
printf "  Test %2d: %-52s " "$SUITE_TOTAL" "file execution preserves exit code"
printf "echo Testing\nexit 29\n" > "$TEST_DIR/test_exit.apex"
"$SHELL_BIN" "$TEST_DIR/test_exit.apex" > /dev/null 2>&1
EXIT_VAL=$?
if [ "$EXIT_VAL" -eq 29 ]; then
    SUITE_PASSED=$((SUITE_PASSED + 1))
    echo -e "${GREEN}[PASS]${NC}"
else
    SUITE_FAILED=$((SUITE_FAILED + 1))
    echo -e "${RED}[FAIL]${NC} (got $EXIT_VAL, expected 29)"
fi

# 7. Sourcing variables and aliases
printf "export SOURCED_VAR=apex_rules\nalias scmd='echo SourcedAlias'\n" > "$TEST_DIR/profile.apex"
run_test "source loads variables and aliases" \
    "source profile.apex
echo \$SOURCED_VAR
scmd" \
    "apex_rules
SourcedAlias" \
    "contains"

# 8. Dot (.) notation synonym
printf "export DOT_VAR=dot_success\n" > "$TEST_DIR/dot_profile.apex"
run_test "dot command is synonym for source" \
    ". dot_profile.apex
echo \$DOT_VAR" \
    "dot_success" \
    "exact"

# 9. Sourcing without args
run_test "source without args reports usage" \
    "source" \
    "source: filename argument required" \
    "contains"

# 10. History displays recorded commands
run_test "history displays recorded commands" \
    "echo hist_test_alpha
echo hist_test_beta
history" \
    "hist_test_alpha" \
    "contains"

# 11. History limits entries
run_test "history limits entries with numeric argument" \
    "echo h1
echo h2
echo h3
history 1" \
    "history 1" \
    "contains"

# 12. History expansion: !! repeats last command
run_test "history expansion !! repeats previous command" \
    "echo initial_expansion_test
!!
" \
    "initial_expansion_test" \
    "contains"

# 13. History expansion: !$ repeats last argument
run_test "history expansion !\$ repeats last argument" \
    "echo apple orange
echo !\$
" \
    "orange" \
    "contains"

suite_summary "Scripting, Sourcing & History"
exit $?
