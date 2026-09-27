#!/usr/bin/env bash
# ==============================================================================
# Suite: Observability & Telemetry (tests/integration/test_observability.sh)
# ==============================================================================

set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../helpers/test_framework.sh"
setup_test_env

echo -e "${BLUE}▶ Running Suite: Systems Observability & Telemetry${NC}"

# 1. sysinfo dashboard
run_test "sysinfo dashboard" \
    "sysinfo" \
    "Apex System Observability" \
    "contains"

# 2. time profiler
run_test "time profiles command execution" \
    "time echo profiling_test" \
    "Execution Telemetry" \
    "contains"

# 3. time exit status preservation
run_test "time preserves command exit status" \
    "time false ; echo \$?" \
    "1" \
    "contains"

# 4. time with pipeline
run_test "time with pipeline" \
    "time echo telemetry | tr a-z A-Z" \
    "TELEMETRY" \
    "contains"

# 5. time without command reports usage
run_test "time without command reports usage" \
    "time" \
    "time: missing command to profile" \
    "contains"

suite_summary "Observability & Telemetry"
exit $?
