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

suite_summary "Expansions & Quoting"
exit $?
