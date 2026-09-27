#!/usr/bin/env bash
# ==============================================================================
# Suite: Algorithmic Intelligence & Typo Correction (tests/integration/test_fuzzy.sh)
# ==============================================================================

set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../helpers/test_framework.sh"
setup_test_env

echo -e "${BLUE}▶ Running Suite: Algorithmic Intelligence & Typo Correction${NC}"

# 1. Suggest built-in command
run_test "suggests built-in command (pwdd -> pwd)" \
    "pwdd" \
    "Did you mean: 'pwd'?" \
    "contains"

# 2. Suggest PATH command for transposition (gti -> git)
run_test "suggests PATH command for transposition (gti -> git)" \
    "gti" \
    "Did you mean: 'git'?" \
    "contains"

# 3. Suggest command for deletion typo (clea -> clear)
run_test "suggests command for deletion typo (clea -> clear)" \
    "clea" \
    "Did you mean: 'clear'?" \
    "contains"

# 4. Command not found returns POSIX exit code 127
run_test "command not found returns exit code 127" \
    "nonexistentcommand12345 ; echo \$?" \
    "127" \
    "contains"

# 5. Completely unknown command outputs error cleanly
run_test "completely unknown command outputs error cleanly" \
    "zzzzqqqq9999" \
    "apex-shell: 'zzzzqqqq9999' command not found" \
    "contains"

suite_summary "Algorithmic Intelligence"
exit $?
