#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "fuzzy.h"
#include "builtins.h"

#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define NC    "\033[0m"

static int g_tests = 0;
static int g_passed = 0;

/* Mock builtins table for unit testing */
char *builtin_str[] = {
    "cd", "pwd", "help", "exit", "jobs", "export", "unset", "env"
};

int lsh_num_builtins(void) {
    return (int)(sizeof(builtin_str) / sizeof(builtin_str[0]));
}

static void assert_dist(const char *s1, const char *s2, int expected) {
    g_tests++;
    int actual = levenshtein_distance(s1, s2);
    if (actual == expected) {
        g_passed++;
        printf("  Unit %2d: dist(\"%-10s\", \"%-10s\") == %d %s[PASS]%s\n",
               g_tests, s1, s2, expected, GREEN, NC);
    } else {
        printf("  Unit %2d: dist(\"%-10s\", \"%-10s\") expected %d, got %d %s[FAIL]%s\n",
               g_tests, s1, s2, expected, actual, RED, NC);
    }
}

static void assert_suggestion(const char *input, const char *expected_suggestion) {
    g_tests++;
    char *match = find_closest_command(input, 2);
    int ok = 0;
    if (expected_suggestion == NULL && match == NULL) {
        ok = 1;
    } else if (expected_suggestion != NULL && match != NULL && strcmp(match, expected_suggestion) == 0) {
        ok = 1;
    }

    if (ok) {
        g_passed++;
        printf("  Unit %2d: suggestion(\"%-10s\") -> \"%-10s\" %s[PASS]%s\n",
               g_tests, input, expected_suggestion ? expected_suggestion : "(null)", GREEN, NC);
    } else {
        printf("  Unit %2d: suggestion(\"%-10s\") expected \"%s\", got \"%s\" %s[FAIL]%s\n",
               g_tests, input,
               expected_suggestion ? expected_suggestion : "(null)",
               match ? match : "(null)", RED, NC);
    }
    if (match) free(match);
}

int main(void) {
    printf("▶ Running C Unit Tests: fuzzy (Damerau-Levenshtein)\n");

    // Exact matches
    assert_dist("git", "git", 0);
    assert_dist("pwd", "pwd", 0);
    assert_dist("", "", 0);

    // Empty vs non-empty
    assert_dist("", "a", 1);
    assert_dist("abc", "", 3);

    // Single character edits
    assert_dist("pwdd", "pwd", 1);    // Deletion / insertion
    assert_dist("pwd", "pwdd", 1);
    assert_dist("clea", "clear", 1);
    assert_dist("cat", "car", 1);     // Substitution

    // Transposition (Damerau-Levenshtein)
    assert_dist("gti", "git", 1);
    assert_dist("sl", "ls", 1);

    // Multi-character edits
    assert_dist("kittens", "sitting", 3);
    assert_dist("completely", "different", 8);

    // Command candidate matching with mock builtins
    assert_suggestion("pwdd", "pwd");
    assert_suggestion("cdd", "cd");
    assert_suggestion("exitt", "exit");
    assert_suggestion("zzzzqqqq9999", NULL);

    if (g_passed == g_tests) {
        printf("  %s✓ All %d fuzzy unit tests passed.%s\n", GREEN, g_tests, NC);
        return 0;
    }
    printf("  %s✗ %d/%d fuzzy unit tests failed.%s\n", RED, g_tests - g_passed, g_tests, NC);
    return 1;
}
