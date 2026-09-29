#!/usr/bin/env bash
# ==============================================================================
# Suite: Expansions, Substitutions & Quoting (tests/integration/test_substitutions.sh)
# ==============================================================================

set -u
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../helpers/test_framework.sh"
setup_test_env

echo -e "${BLUE}▶ Running Suite: Expansions & Quoting${NC}"

# 1. Double quotes preserve spaces
run_test "double quotes preserve inner spaces" \
    "echo \"hello   world   from   c-shell\"" \
    "hello   world   from   c-shell" \
    "exact"

# 2. Single quotes preserve spaces and literals
run_test "single quotes preserve inner spaces" \
    "echo 'single quoted argument with spaces'" \
    "single quoted argument with spaces" \
    "exact"

# 3. $? exit code
run_test "\$? reflects previous command status" \
    "false ; echo \$? ; true ; echo \$?" \
    "1
0" \
    "exact"

# 4. $VAR and single quote preservation
run_test "\$VAR expands and single quotes preserve" \
    "echo \$USER
echo '\$USER'" \
    "$USER
\$USER" \
    "exact"

# 5. Tilde expansion
run_test "tilde (~) expands to HOME" \
    "echo ~" \
    "$HOME" \
    "exact"

# 6. Braced ${VAR} expansion
run_test "braced \${VAR} inside text" \
    "export APEX_PREFIX=Super
echo \${APEX_PREFIX}_Shell" \
    "Super_Shell" \
    "exact"

# 7. Command substitution $() basic
run_test "basic \$() captures stdout" \
    "echo Hello \$(echo World)" \
    "Hello World" \
    "exact"

# 8. Command substitution $() with pipeline
run_test "pipeline inside \$()" \
    "echo Result: \$(echo 'apex shell' | tr a-z A-Z)" \
    "Result: APEX SHELL" \
    "exact"

# 9. Command substitution inside double quotes
run_test "substitution inside double quotes" \
    "echo \"Sub: \$(echo nested_output)\"" \
    "Sub: nested_output" \
    "exact"

# 10. Command substitution backticks
run_test "backticks command execution" \
    "echo Backtick: \`echo backtick_works\`" \
    "Backtick: backtick_works" \
    "exact"

# 11. Arithmetic Expansion: basic addition
run_test "arithmetic expansion: basic addition" \
    "echo \$(( 3 + 5 ))" \
    "8" \
    "exact"

# 12. Arithmetic Expansion: precedence and parentheses
run_test "arithmetic expansion: precedence and parentheses" \
    "echo \$(( (2 + 3) * 4 ))" \
    "20" \
    "exact"

# 13. Arithmetic Expansion: division and modulo
run_test "arithmetic expansion: division and modulo" \
    "echo \$(( 100 / 4 )) \$(( 17 % 5 ))" \
    "25 2" \
    "exact"

# 14. Arithmetic Expansion: variables and inside double quotes
run_test "arithmetic expansion: variables and inside double quotes" \
    "export APEX_NUM=7
echo \"Result: \$(( APEX_NUM * 6 ))\"" \
    "Result: 42" \
    "exact"

# 15. Wildcard Globbing: * expands matching files
touch "$TEST_DIR/alpha1.test" "$TEST_DIR/alpha2.test"
run_test "wildcard globbing (*) expands matching files" \
    "echo alpha*.test" \
    "alpha1.test alpha2.test" \
    "contains"

# 16. Wildcard Globbing: ? matches single character
touch "$TEST_DIR/beta1.test" "$TEST_DIR/beta2.test"
run_test "wildcard globbing (?) matches single character" \
    "echo beta?.test" \
    "beta1.test beta2.test" \
    "contains"

# 17. Quoted Glob: single quotes prevent glob expansion
run_test "single quotes prevent wildcard globbing" \
    "echo 'alpha*.test'" \
    "alpha*.test" \
    "exact"

# 18. Non-matching pattern preserves literal pattern
run_test "unmatched wildcard pattern preserved literally" \
    "echo unmatched_glob_*.xyz" \
    "unmatched_glob_*.xyz" \
    "exact"

suite_summary "Expansions & Quoting"
exit $?
