#!/usr/bin/env bash
# ==============================================================================
# Suite: Safety Shield & Command Aliases (tests/integration/test_safety.sh)
# ==============================================================================

set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../helpers/test_framework.sh"
setup_test_env

echo -e "${BLUE}▶ Running Suite: Safety Shield & Aliases${NC}"

# 1. Define and execute alias
run_test "define and execute alias" \
    "alias greet='echo HelloApex'
greet" \
    "HelloApex" \
    "exact"

# 2. List active aliases
run_test "list active aliases" \
    "alias myecho='echo'
alias" \
    "alias myecho='echo'" \
    "contains"

# 3. Unalias removes alias
run_test "unalias removes alias" \
    "alias temp='echo Temp'
unalias temp
temp" \
    "apex-shell: 'temp' command not found" \
    "contains"

# 4. Alias with trailing arguments
printf "SampleContent\n" > "$TEST_DIR/sample.txt"
run_test "alias with trailing arguments" \
    "alias show='cat'
show sample.txt" \
    "SampleContent" \
    "exact"

# 5. Safemode displays status
run_test "safemode displays status" \
    "safemode" \
    "Safety Shield: ENABLED" \
    "contains"

# 6. Safemode blocks dangerous rm -rf /
run_test "blocks dangerous rm -rf /" \
    "rm -rf /" \
    "Blocked destructive recursive deletion on '/'" \
    "contains"

# 7. Safemode toggle off and on
run_test "safemode toggle off and on" \
    "safemode off
safemode on" \
    "Safety Shield: DISABLED
Safety Shield: ENABLED" \
    "exact"

suite_summary "Safety Shield & Aliases"
exit $?
