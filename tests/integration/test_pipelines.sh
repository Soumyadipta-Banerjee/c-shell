#!/usr/bin/env bash
# ==============================================================================
# Suite: Pipelines, Redirections & Chaining (tests/integration/test_pipelines.sh)
# ==============================================================================

set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../helpers/test_framework.sh"
setup_test_env

echo -e "${BLUE}▶ Running Suite: Pipelines, Redirection & Operators${NC}"

# 1. Output redirection >
run_test "output redirection (>) writes file" \
    "echo \"output test\" > out.txt
cat out.txt" \
    "output test" \
    "exact"

# 2. Append redirection >>
run_test "append redirection (>>) adds content" \
    "echo \"first line\" > append.txt
echo \"second line\" >> append.txt
cat append.txt" \
    "first line
second line" \
    "exact"

# 3. Input redirection <
echo "input file content" > "$TEST_DIR/input.txt"
run_test "input redirection (<) reads file" \
    "cat < input.txt" \
    "input file content" \
    "exact"

# 4. Combined < and >
echo "data to copy" > "$TEST_DIR/source.txt"
run_test "combined input (<) and output (>)" \
    "cat < source.txt > dest.txt
cat dest.txt" \
    "data to copy" \
    "exact"

# 5. 2-stage pipeline
run_test "2-stage pipeline (echo | tr)" \
    "echo \"pipeline test\" | tr \"a-z\" \"A-Z\"" \
    "PIPELINE TEST" \
    "exact"

# 6. 3-stage pipeline
run_test "3-stage pipeline (echo | tr | wc -l)" \
    "echo \"one two three four\" | tr \" \" \"\n\" | wc -l" \
    "4" \
    "exact"

# 7. Pipeline filtering with grep
run_test "filtering stream with grep" \
    "printf \"apple\nbanana\ncherry\n\" | grep banana" \
    "banana" \
    "exact"

# 8. Redirection syntax error
run_test "syntax error when redirection target missing" \
    "echo test >" \
    "syntax error near unexpected token 'newline'" \
    "contains"

# 9. Pipeline syntax error
run_test "syntax error on invalid empty pipe stage" \
    "ls | | grep" \
    "syntax error near unexpected token '|'" \
    "contains"

# 10. Chaining semicolon ;
run_test "semicolon (;) executes in sequence" \
    "echo first; echo second; echo third" \
    "first
second
third" \
    "exact"

# 11. Chaining AND && success
run_test "AND (&&) executes next on success" \
    "true && echo and_ok" \
    "and_ok" \
    "exact"

# 12. Chaining AND && failure
run_test "AND (&&) skips next on failure" \
    "false && echo unreachable || echo reached" \
    "reached" \
    "exact"

# 13. Chaining OR || failure
run_test "OR (||) executes on failure" \
    "false || echo fallback" \
    "fallback" \
    "exact"

# 14. Chaining OR || success
run_test "OR (||) skips next on success" \
    "true || echo unreachable
echo after" \
    "after" \
    "exact"

# 15. Extended Redirection: stderr redirection 2>
run_test "stderr redirection (2>) writes error to file" \
    "cat nonexistent_file_xyz 2> err.txt
cat err.txt" \
    "No such file or directory" \
    "contains"

# 16. Extended Redirection: stderr append redirection 2>>
run_test "stderr append redirection (2>>) appends to file" \
    "echo initial > append_err.txt
cat nonexistent_file_xyz 2>> append_err.txt
cat append_err.txt" \
    "initial" \
    "contains"

# 17. Extended Redirection: &> redirects stdout and stderr
run_test "combined output redirection (&>) writes stdout and stderr" \
    "echo standard_out &> all.txt
cat all.txt" \
    "standard_out" \
    "exact"

# 18. Extended Redirection: 2>&1 merges stderr into stdout
run_test "merging stderr to stdout (2>&1) captured in pipeline" \
    "cat nonexistent_file_xyz 2>&1 | grep -o 'No such file'" \
    "No such file" \
    "exact"

# 19. Herestring: <<< injects string into stdin
run_test "herestring (<<<) pipes string to command" \
    "cat <<< 'hello herestring'" \
    "hello herestring" \
    "exact"

# 20. Herestring in pipeline
run_test "herestring in pipeline" \
    "tr a-z A-Z <<< 'apex shell' | grep APEX" \
    "APEX SHELL" \
    "exact"

# 21. Heredoc: << EOF captures multi-line content
run_test "heredoc (<<) captures multi-line stream" \
    "cat << EOF
first line
second line
EOF" \
    "first line
second line" \
    "contains"

# 22. Heredoc inside pipeline
run_test "heredoc with pipeline processing" \
    "grep -E 'one|three' << EOF | wc -l
one
two
three
EOF" \
    "2" \
    "exact"

suite_summary "Pipelines & Operators"
exit $?
