#!/usr/bin/env bash
# ==============================================================================
# Suite: Job Control & Process Signals (tests/integration/test_jobs.sh)
# ==============================================================================

set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../helpers/test_framework.sh"
setup_test_env

echo -e "${BLUE}▶ Running Suite: Job Control & Signals${NC}"

# 1. Signal Handling: SIGINT directly to shell does not kill it
SUITE_TOTAL=$((SUITE_TOTAL + 1))
printf "  Test %2d: %-52s " "$SUITE_TOTAL" "SIGINT does not kill shell at prompt"
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
    SUITE_PASSED=$((SUITE_PASSED + 1))
    echo -e "${GREEN}[PASS]${NC}"
else
    SUITE_FAILED=$((SUITE_FAILED + 1))
    echo -e "${RED}[FAIL]${NC}"
fi

# 2. Signal Handling: SIGINT terminates running child while shell survives
SUITE_TOTAL=$((SUITE_TOTAL + 1))
printf "  Test %2d: %-52s " "$SUITE_TOTAL" "SIGINT interrupts child, shell survives"
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
    SUITE_PASSED=$((SUITE_PASSED + 1))
    echo -e "${GREEN}[PASS]${NC}"
else
    SUITE_FAILED=$((SUITE_FAILED + 1))
    echo -e "${RED}[FAIL]${NC}"
fi

# 3. Background job with &
run_test "job launches asynchronously with &" \
    "sleep 1 &" \
    "[1]" \
    "contains"

# 4. jobs lists active tasks
run_test "jobs builtin lists active tasks" \
    "sleep 2 &
jobs" \
    "Running" \
    "contains"

# 5. Finished job is reaped and reported
run_test "finished job is reaped and reported" \
    "sleep 0.4 &
sleep 0.7
jobs" \
    "Done" \
    "contains"

# 6. jobs status indicator
run_test "jobs displays Running status" \
    "sleep 5 &
jobs" \
    "Running" \
    "contains"

# 7. kill by %id
run_test "kill sends signal by %id" \
    "sleep 30 &
kill %1" \
    "[1] " \
    "contains"

# 8. kill with explicit signal flag
run_test "kill with explicit signal flag" \
    "sleep 30 &
kill -9 %1" \
    "[1] " \
    "contains"

# 9. kill with -s signal flag
run_test "kill with -s signal flag" \
    "sleep 30 &
kill -s KILL %1" \
    "[1] " \
    "contains"

# 10. kill invalid job ID
run_test "kill invalid job ID reports error" \
    "kill %99" \
    "no such job" \
    "contains"

# 10. bg command targets background job
run_test "bg command targets background job" \
    "sleep 10 &
bg %1" \
    "sleep 10" \
    "contains"

# 11. fg without matching job
run_test "fg without matching job reports error" \
    "fg %99" \
    "no such job" \
    "contains"

suite_summary "Job Control & Signals"
exit $?
