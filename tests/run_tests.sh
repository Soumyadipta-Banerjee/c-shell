#!/usr/bin/env bash
# ==============================================================================
# Apex Shell Master Test Orchestrator (tests/run_tests.sh)
# Dispatches modular integration and unit test suites
# ==============================================================================

set -u

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

SHELL_BIN="$PROJECT_ROOT/apex-shell"
if [ ! -x "$SHELL_BIN" ]; then
    if [ -x "$PROJECT_ROOT/c-shell" ]; then
        SHELL_BIN="$PROJECT_ROOT/c-shell"
    else
        echo -e "${RED}Error: Shell binary not found at '$SHELL_BIN'. Run 'make' first.${NC}"
        exit 1
    fi
fi

FAST_MODE=0
TARGET_SUITE=""
RUN_UNIT=0

# Parse CLI arguments
for arg in "$@"; do
    case "$arg" in
        --fast|-f)
            FAST_MODE=1
            ;;
        --unit|-u)
            RUN_UNIT=1
            ;;
        --all|-a)
            RUN_UNIT=1
            ;;
        *)
            TARGET_SUITE="$arg"
            ;;
    esac
done

echo -e "${BLUE}================================================================${NC}"
echo -e "${BOLD}${CYAN}                Apex Shell Test Suite Runner                   ${NC}"
echo -e "${BLUE}================================================================${NC}"

TOTAL_SUITES=0
PASSED_SUITES=0
FAILED_SUITES=0

run_single_suite() {
    local suite_script="$1"
    local suite_name
    suite_name="$(basename "$suite_script" .sh)"

    TOTAL_SUITES=$((TOTAL_SUITES + 1))
    echo ""
    bash "$suite_script"
    local status=$?

    if [ "$status" -eq 0 ]; then
        PASSED_SUITES=$((PASSED_SUITES + 1))
    else
        FAILED_SUITES=$((FAILED_SUITES + 1))
    fi
}

# If a specific suite is requested:
if [ -n "$TARGET_SUITE" ]; then
    MATCHING_SUITE=""
    for s in "$SCRIPT_DIR"/integration/test_*.sh; do
        bname="$(basename "$s" .sh)"
        if [[ "$bname" == *"$TARGET_SUITE"* ]]; then
            MATCHING_SUITE="$s"
            break
        fi
    done

    if [ -n "$MATCHING_SUITE" ] && [ -f "$MATCHING_SUITE" ]; then
        run_single_suite "$MATCHING_SUITE"
    else
        echo -e "${RED}Error: No suite matching '${TARGET_SUITE}' found in tests/integration/${NC}"
        echo "Available suites:"
        for s in "$SCRIPT_DIR"/integration/test_*.sh; do
            echo "  - $(basename "$s" .sh | sed 's/test_//')"
        done
        exit 1
    fi
else
    # Run integration suites
    for suite in \
        "$SCRIPT_DIR/integration/test_builtins.sh" \
        "$SCRIPT_DIR/integration/test_pipelines.sh" \
        "$SCRIPT_DIR/integration/test_substitutions.sh" \
        "$SCRIPT_DIR/integration/test_safety.sh" \
        "$SCRIPT_DIR/integration/test_observability.sh" \
        "$SCRIPT_DIR/integration/test_fuzzy.sh" \
        "$SCRIPT_DIR/integration/test_scripting.sh"; do
        if [ -f "$suite" ]; then
            run_single_suite "$suite"
        fi
    done

    # Run jobs suite (unless --fast is specified)
    if [ "$FAST_MODE" -eq 1 ]; then
        echo -e "\n${YELLOW}⚡ Fast mode: Skipping test_jobs.sh (sleep tests).${NC}"
    else
        if [ -f "$SCRIPT_DIR/integration/test_jobs.sh" ]; then
            run_single_suite "$SCRIPT_DIR/integration/test_jobs.sh"
        fi
    fi

    # Run unit tests if requested
    if [ "$RUN_UNIT" -eq 1 ]; then
        if [ -x "$SCRIPT_DIR/unit/test_fuzzy" ]; then
            TOTAL_SUITES=$((TOTAL_SUITES + 1))
            echo -e "\n${BLUE}▶ Running C Unit Suite: fuzzy algorithm${NC}"
            "$SCRIPT_DIR/unit/test_fuzzy"
            if [ $? -eq 0 ]; then
                PASSED_SUITES=$((PASSED_SUITES + 1))
            else
                FAILED_SUITES=$((FAILED_SUITES + 1))
            fi
        fi

        if [ -x "$SCRIPT_DIR/unit/test_alias" ]; then
            TOTAL_SUITES=$((TOTAL_SUITES + 1))
            echo -e "\n${BLUE}▶ Running C Unit Suite: alias table${NC}"
            "$SCRIPT_DIR/unit/test_alias"
            if [ $? -eq 0 ]; then
                PASSED_SUITES=$((PASSED_SUITES + 1))
            else
                FAILED_SUITES=$((FAILED_SUITES + 1))
            fi
        fi
    fi
fi

echo -e "\n${BLUE}================================================================${NC}"
if [ "$FAILED_SUITES" -eq 0 ]; then
    echo -e "${GREEN}${BOLD}✓ ALL TEST SUITES PASSED (${PASSED_SUITES}/${TOTAL_SUITES} suites successful)${NC}"
    echo -e "${BLUE}================================================================${NC}"
    exit 0
else
    echo -e "${RED}${BOLD}✗ TEST RUN FAILED (${FAILED_SUITES}/${TOTAL_SUITES} suites failed)${NC}"
    echo -e "${BLUE}================================================================${NC}"
    exit 1
fi
