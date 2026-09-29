#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include "arithmetic.h"

#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define NC    "\033[0m"

static int g_tests = 0;
static int g_passed = 0;

static void assert_math(const char *expr, long long expected) {
    g_tests++;
    int err = 0;
    long long actual = evaluate_arithmetic_expression(expr, &err);
    if (!err && actual == expected) {
        g_passed++;
        printf("  Unit %2d: %-32s == %4lld %s[PASS]%s\n", g_tests, expr, expected, GREEN, NC);
    } else {
        printf("  Unit %2d: %-32s expected %lld, got %lld (err=%d) %s[FAIL]%s\n",
               g_tests, expr, expected, actual, err, RED, NC);
    }
}

static void assert_math_error(const char *expr) {
    g_tests++;
    int err = 0;
    evaluate_arithmetic_expression(expr, &err);
    if (err) {
        g_passed++;
        printf("  Unit %2d: %-32s -> error detected %s[PASS]%s\n", g_tests, expr, GREEN, NC);
    } else {
        printf("  Unit %2d: %-32s expected error, but succeeded %s[FAIL]%s\n", g_tests, expr, RED, NC);
    }
}

int main(void) {
    printf("▶ Running C Unit Tests: arithmetic expansion engine\n");

    // Basic arithmetic
    assert_math("3 + 5", 8);
    assert_math("10 - 4", 6);
    assert_math("6 * 7", 42);
    assert_math("15 / 3", 5);
    assert_math("17 % 5", 2);

    // Operator precedence
    assert_math("2 + 3 * 4", 14);
    assert_math("3 * 4 + 2", 14);

    // Parentheses
    assert_math("(2 + 3) * 4", 20);
    assert_math("100 / (2 + 3)", 20);

    // Unary operators
    assert_math("-5 + 10", 5);
    assert_math("+7", 7);
    assert_math("!0", 1);
    assert_math("!5", 0);

    // Comparisons
    assert_math("5 > 3", 1);
    assert_math("3 > 5", 0);
    assert_math("4 >= 4", 1);
    assert_math("4 <= 4", 1);
    assert_math("7 == 7", 1);
    assert_math("7 != 8", 1);

    // Logical AND/OR
    assert_math("(5 > 2) && (3 == 3)", 1);
    assert_math("(5 < 2) || (3 == 3)", 1);

    // Variable lookup
    setenv("APEX_MATH_A", "15", 1);
    setenv("APEX_MATH_B", "5", 1);
    assert_math("APEX_MATH_A + APEX_MATH_B", 20);
    assert_math("$APEX_MATH_A / $APEX_MATH_B", 3);

    // Errors: division / modulo by zero and syntax errors
    assert_math_error("10 / 0");
    assert_math_error("10 % 0");
    assert_math_error("(3 + 4");

    if (g_passed == g_tests) {
        printf("  %s✓ All %d arithmetic unit tests passed.%s\n", GREEN, g_tests, NC);
        return 0;
    }
    printf("  %s✗ %d/%d arithmetic unit tests failed.%s\n", RED, g_tests - g_passed, g_tests, NC);
    return 1;
}
