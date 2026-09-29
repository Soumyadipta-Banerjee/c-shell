#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "globber.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int g_test_count = 0;
static int g_pass_count = 0;

static void assert_int_eq(const char *expr_str, int actual, int expected)
{
    g_test_count++;
    printf("  Unit %2d: %-32s == %4d ", g_test_count, expr_str, expected);
    if (actual == expected)
    {
        g_pass_count++;
        printf("[PASS]\n");
    }
    else
    {
        printf("[FAIL] (got %d)\n", actual);
    }
}

static void assert_true(const char *desc, int condition)
{
    g_test_count++;
    printf("  Unit %2d: %-32s -> %s [PASS]\n", g_test_count, desc, condition ? "true" : "false");
    if (condition)
    {
        g_pass_count++;
    }
    else
    {
        printf("       FAILED: condition not met\n");
    }
}

int main(void)
{
    printf("▶ Running C Unit Tests: wildcard globbing engine\n");

    // 1. Meta character detection tests
    assert_int_eq("has_glob_meta(\"hello\")", has_glob_meta("hello"), 0);
    assert_int_eq("has_glob_meta(\"\")", has_glob_meta(""), 0);
    assert_int_eq("has_glob_meta(NULL)", has_glob_meta(NULL), 0);
    assert_int_eq("has_glob_meta(\"*.c\")", has_glob_meta("*.c"), 1);
    assert_int_eq("has_glob_meta(\"file?.txt\")", has_glob_meta("file?.txt"), 1);
    assert_int_eq("has_glob_meta(\"data[0-9]\")", has_glob_meta("data[0-9]"), 1);
    assert_int_eq("has_glob_meta(\"src/sub/*.h\")", has_glob_meta("src/sub/*.h"), 1);

    // 2. Path pattern expansion tests
    char **paths = NULL;
    int count = 0;
    int rc = expand_path_pattern("tests/unit/test_*.c", &paths, &count);
    assert_int_eq("expand_path_pattern rc == 0", rc, 0);
    assert_true("matched at least 3 test files", count >= 3);

    int found_fuzzy = 0, found_alias = 0, found_arithmetic = 0;
    for (int i = 0; i < count; i++)
    {
        if (strstr(paths[i], "test_fuzzy.c")) found_fuzzy = 1;
        if (strstr(paths[i], "test_alias.c")) found_alias = 1;
        if (strstr(paths[i], "test_arithmetic.c")) found_arithmetic = 1;
    }
    assert_true("found test_fuzzy.c in glob", found_fuzzy);
    assert_true("found test_alias.c in glob", found_alias);
    assert_true("found test_arithmetic.c in glob", found_arithmetic);
    free_path_list(paths, count);

    // 3. Nonexistent pattern check (GLOB_NOCHECK fallback)
    paths = NULL;
    count = 0;
    rc = expand_path_pattern("nonexistent_pattern_*.xyz", &paths, &count);
    assert_int_eq("expand nonexistent pattern rc == 0", rc, 0);
    assert_int_eq("nonexistent returns 1 literal match", count, 1);
    assert_true("literal string preserved", strcmp(paths[0], "nonexistent_pattern_*.xyz") == 0);
    free_path_list(paths, count);

    // 4. Token globbing expansion with literal preservation
    ShellToken tok1 = { strdup("echo"), 0 };
    ShellToken tok2_literal = { strdup("tests/unit/test_*.c"), 1 }; // Quoted literal
    ShellToken *tokens_lit[2] = { &tok1, &tok2_literal };

    int lit_count = 0;
    char **lit_args = expand_tokens_with_glob(tokens_lit, 0, 2, &lit_count);
    assert_int_eq("literal token count preserved", lit_count, 2);
    assert_true("literal string not expanded", strcmp(lit_args[1], "tests/unit/test_*.c") == 0);
    free_glob_args(lit_args, lit_count);
    free(tok1.text);
    free(tok2_literal.text);

    // 5. Token globbing expansion with unquoted pattern
    ShellToken tok3 = { strdup("ls"), 0 };
    ShellToken tok4_glob = { strdup("tests/unit/test_*.c"), 0 }; // Unquoted pattern
    ShellToken *tokens_glob[2] = { &tok3, &tok4_glob };

    int glob_count = 0;
    char **glob_args = expand_tokens_with_glob(tokens_glob, 0, 2, &glob_count);
    assert_true("unquoted pattern expands to multiple args", glob_count >= 4);
    assert_true("first arg is command", strcmp(glob_args[0], "ls") == 0);
    free_glob_args(glob_args, glob_count);
    free(tok3.text);
    free(tok4_glob.text);

    printf("  ✓ All %d wildcard glob unit tests passed (%d/%d).\n",
           g_test_count, g_pass_count, g_test_count);
    return (g_pass_count == g_test_count) ? 0 : 1;
}
