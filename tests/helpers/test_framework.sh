#!/usr/bin/env bash
# ==============================================================================
# Apex Shell Test Framework (tests/helpers/test_framework.sh)
# Reusable assertion library and test execution harness
# ==============================================================================

set -u

# Terminal Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

SUITE_TOTAL=0
SUITE_PASSED=0
SUITE_FAILED=0

# Locate project root and apex-shell binary
setup_test_env() {
    local helper_dir
    helper_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    PROJECT_ROOT="$(cd "$helper_dir/../.." && pwd)"
    SHELL_BIN="$PROJECT_ROOT/apex-shell"

    if [ ! -x "$SHELL_BIN" ]; then
        if [ -x "$PROJECT_ROOT/c-shell" ]; then
            SHELL_BIN="$PROJECT_ROOT/c-shell"
        else
            echo -e "${RED}Error: Shell binary not found or not executable at '$SHELL_BIN'.${NC}"
            echo "Please run 'make' first."
            exit 1
        fi
    fi

    # Create temporary sandbox directory for test isolation
    TEST_DIR="$(mktemp -d -t apex_test_XXXXXX)"
    trap 'rm -rf "$TEST_DIR"' EXIT

    FIXTURES_DIR="$PROJECT_ROOT/tests/fixtures"
}

# Run a test by passing input string into shell's stdin
run_test() {
    local test_name="$1"
    local input="$2"
    local expected_output="$3"
    local check_type="${4:-exact}" # exact, contains, not_contains, exit_only, or non_zero

    SUITE_TOTAL=$((SUITE_TOTAL + 1))
    printf "  Test %2d: %-52s " "$SUITE_TOTAL" "$test_name"

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
            if echo "$actual_output" | grep -F -q "$expected_output"; then
                pass=1
            fi
            ;;
        not_contains)
            if ! echo "$actual_output" | grep -F -q "$expected_output"; then
                pass=1
            fi
            ;;
        exit_only)
            if [ "$exit_code" -eq 0 ]; then
                pass=1
            fi
            ;;
        non_zero)
            if [ "$exit_code" -ne 0 ]; then
                pass=1
            fi
            ;;
    esac

    if [ "$pass" -eq 1 ]; then
        SUITE_PASSED=$((SUITE_PASSED + 1))
        echo -e "${GREEN}[PASS]${NC}"
    else
        SUITE_FAILED=$((SUITE_FAILED + 1))
        echo -e "${RED}[FAIL]${NC}"
        echo "    Input:"
        echo "$input" | sed 's/^/      /'
        echo "    Expected ($check_type):"
        echo "$expected_output" | sed 's/^/      /'
        echo "    Actual:"
        echo "$actual_output" | sed 's/^/      /'
    fi
}

# Run a test command directly (e.g. apex-shell -c "...")
run_test_cmd() {
    local test_name="$1"
    local cmd_args="$2"
    local expected_output="$3"
    local check_type="${4:-exact}" # exact, contains, or exit_code

    SUITE_TOTAL=$((SUITE_TOTAL + 1))
    printf "  Test %2d: %-52s " "$SUITE_TOTAL" "$test_name"

    local actual_output
    actual_output=$(cd "$TEST_DIR" && eval "\"$SHELL_BIN\" $cmd_args" 2>&1)
    local exit_code=$?

    local pass=0
    case "$check_type" in
        exact)
            if [ "$actual_output" = "$expected_output" ]; then
                pass=1
            fi
            ;;
        contains)
            if echo "$actual_output" | grep -F -q "$expected_output"; then
                pass=1
            fi
            ;;
        exit_code)
            if [ "$exit_code" -eq "$expected_output" ]; then
                pass=1
            fi
            ;;
    esac

    if [ "$pass" -eq 1 ]; then
        SUITE_PASSED=$((SUITE_PASSED + 1))
        echo -e "${GREEN}[PASS]${NC}"
    else
        SUITE_FAILED=$((SUITE_FAILED + 1))
        echo -e "${RED}[FAIL]${NC}"
        echo "    Command: $SHELL_BIN $cmd_args"
        echo "    Expected ($check_type): $expected_output"
        echo "    Actual Output: $actual_output (exit code $exit_code)"
    fi
}

# Print summary of the current test suite
suite_summary() {
    local suite_name="$1"
    if [ "$SUITE_FAILED" -eq 0 ]; then
        echo -e "  ${GREEN}✓ Suite '${suite_name}' passed: ${SUITE_PASSED}/${SUITE_TOTAL} tests successful.${NC}"
        return 0
    else
        echo -e "  ${RED}✗ Suite '${suite_name}' failed: ${SUITE_FAILED}/${SUITE_TOTAL} tests failed.${NC}"
        return 1
    fi
}
