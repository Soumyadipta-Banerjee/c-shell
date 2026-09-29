#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "braces.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GREEN  "\033[0;32m"
#define RED    "\033[0;31m"
#define BOLD   "\033[1m"
#define NC     "\033[0m"

static int g_tests_run = 0;
static int g_tests_passed = 0;

static void assert_test(const char *expr_str, int cond)
{
    g_tests_run++;
    printf("  Unit %2d: %-36s ", g_tests_run, expr_str);
    if (cond)
    {
        printf("%s[PASS]%s\n", GREEN, NC);
        g_tests_passed++;
    }
    else
    {
        printf("%s[FAIL]%s\n", RED, NC);
    }
}

int main(void)
{
    printf("%s▶ Running C Unit Tests: brace expansion engine%s\n", BOLD, NC);

    // 1. Syntax check tests
    assert_test("has_brace_syntax(NULL) == 0", has_brace_syntax(NULL) == 0);
    assert_test("has_brace_syntax(\"hello\") == 0", has_brace_syntax("hello") == 0);
    assert_test("has_brace_syntax(\"{foo}\") == 0", has_brace_syntax("{foo}") == 0);
    assert_test("has_brace_syntax(\"{a,b}\") == 1", has_brace_syntax("{a,b}") == 1);
    assert_test("has_brace_syntax(\"{1..5}\") == 1", has_brace_syntax("{1..5}") == 1);
    assert_test("has_brace_syntax(\"{a..z}\") == 1", has_brace_syntax("{a..z}") == 1);
    assert_test("has_brace_syntax(\"file_{old,new}.c\") == 1", has_brace_syntax("file_{old,new}.c") == 1);

    // 2. Comma list expansion
    char **res = NULL;
    int count = 0;

    int rc = expand_braces("pre_{a,b}_suf", &res, &count);
    assert_test("expand comma list rc == 0", rc == 0);
    assert_test("expand comma count == 2", count == 2);
    assert_test("expand comma item 0 == pre_a_suf", count == 2 && strcmp(res[0], "pre_a_suf") == 0);
    assert_test("expand comma item 1 == pre_b_suf", count == 2 && strcmp(res[1], "pre_b_suf") == 0);
    free_brace_list(res, count);

    // 3. Three items comma list
    res = NULL; count = 0;
    rc = expand_braces("{red,green,blue}", &res, &count);
    assert_test("expand 3 items count == 3", count == 3);
    assert_test("expand 3 items first == red", count == 3 && strcmp(res[0], "red") == 0);
    assert_test("expand 3 items middle == green", count == 3 && strcmp(res[1], "green") == 0);
    assert_test("expand 3 items last == blue", count == 3 && strcmp(res[2], "blue") == 0);
    free_brace_list(res, count);

    // 4. Numeric range ascending
    res = NULL; count = 0;
    rc = expand_braces("{1..4}", &res, &count);
    assert_test("expand {1..4} count == 4", count == 4);
    assert_test("expand {1..4} values [1,2,3,4]", count == 4 &&
                strcmp(res[0], "1") == 0 && strcmp(res[1], "2") == 0 &&
                strcmp(res[2], "3") == 0 && strcmp(res[3], "4") == 0);
    free_brace_list(res, count);

    // 5. Numeric range descending
    res = NULL; count = 0;
    rc = expand_braces("{3..1}", &res, &count);
    assert_test("expand {3..1} count == 3", count == 3);
    assert_test("expand {3..1} values [3,2,1]", count == 3 &&
                strcmp(res[0], "3") == 0 && strcmp(res[1], "2") == 0 &&
                strcmp(res[2], "1") == 0);
    free_brace_list(res, count);

    // 6. Numeric range with step
    res = NULL; count = 0;
    rc = expand_braces("{1..9..3}", &res, &count);
    assert_test("expand {1..9..3} count == 3", count == 3);
    assert_test("expand {1..9..3} values [1,4,7]", count == 3 &&
                strcmp(res[0], "1") == 0 && strcmp(res[1], "4") == 0 &&
                strcmp(res[2], "7") == 0);
    free_brace_list(res, count);

    // 7. Character range ascending
    res = NULL; count = 0;
    rc = expand_braces("{a..c}", &res, &count);
    assert_test("expand {a..c} count == 3", count == 3);
    assert_test("expand {a..c} values [a,b,c]", count == 3 &&
                strcmp(res[0], "a") == 0 && strcmp(res[1], "b") == 0 &&
                strcmp(res[2], "c") == 0);
    free_brace_list(res, count);

    // 8. Character range descending
    res = NULL; count = 0;
    rc = expand_braces("{c..a}", &res, &count);
    assert_test("expand {c..a} count == 3", count == 3);
    assert_test("expand {c..a} values [c,b,a]", count == 3 &&
                strcmp(res[0], "c") == 0 && strcmp(res[1], "b") == 0 &&
                strcmp(res[2], "a") == 0);
    free_brace_list(res, count);

    // 9. Cartesian product: {A,B}{1,2}
    res = NULL; count = 0;
    rc = expand_braces("{A,B}{1,2}", &res, &count);
    assert_test("cartesian {A,B}{1,2} count == 4", count == 4);
    assert_test("cartesian product values", count == 4 &&
                strcmp(res[0], "A1") == 0 && strcmp(res[1], "A2") == 0 &&
                strcmp(res[2], "B1") == 0 && strcmp(res[3], "B2") == 0);
    free_brace_list(res, count);

    // 10. Non-brace literal preservation
    res = NULL; count = 0;
    rc = expand_braces("simple_word", &res, &count);
    assert_test("literal simple_word count == 1", count == 1 && strcmp(res[0], "simple_word") == 0);
    free_brace_list(res, count);

    // 11. Single brace without comma/range preserved
    res = NULL; count = 0;
    rc = expand_braces("{lonely_brace}", &res, &count);
    assert_test("unexpandable brace preserved", count == 1 && strcmp(res[0], "{lonely_brace}") == 0);
    free_brace_list(res, count);

    printf("%s✓ All %d brace expansion unit tests passed (%d/%d).%s\n",
           GREEN, g_tests_run, g_tests_passed, g_tests_run, NC);
    return (g_tests_passed == g_tests_run) ? 0 : 1;
}
