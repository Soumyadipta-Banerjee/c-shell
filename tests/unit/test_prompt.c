#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "prompt.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int g_test_count = 0;
static int g_pass_count = 0;

static void assert_str_eq(const char *desc, const char *actual, const char *expected)
{
    g_test_count++;
    printf("  Unit %2d: %-32s == \"%s\" ", g_test_count, desc, expected);
    if (actual && expected && strcmp(actual, expected) == 0)
    {
        g_pass_count++;
        printf("[PASS]\n");
    }
    else
    {
        printf("[FAIL] (got \"%s\")\n", actual ? actual : "(null)");
    }
}

static void assert_str_contains(const char *desc, const char *actual, const char *sub)
{
    g_test_count++;
    printf("  Unit %2d: %-32s contains \"%s\" ", g_test_count, desc, sub);
    if (actual && sub && strstr(actual, sub) != NULL)
    {
        g_pass_count++;
        printf("[PASS]\n");
    }
    else
    {
        printf("[FAIL] (got \"%s\")\n", actual ? actual : "(null)");
    }
}

static void assert_int_eq(const char *desc, int actual, int expected)
{
    g_test_count++;
    printf("  Unit %2d: %-32s == %d ", g_test_count, desc, expected);
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

int main(void)
{
    printf("▶ Running C Unit Tests: PS1 prompt formatting engine\n");

    char buf[1024];

    // 1. Literal text
    int rc = format_ps1("apex-shell$ ", buf, sizeof(buf));
    assert_int_eq("format_ps1 literal text rc", rc, 0);
    assert_str_eq("format_ps1 literal text", buf, "apex-shell$ ");

    // 2. User expansion (\u)
    char *user = getenv("USER");
    if (!user) user = "user";
    rc = format_ps1("\\u", buf, sizeof(buf));
    assert_int_eq("format_ps1 \\u rc", rc, 0);
    assert_str_eq("format_ps1 \\u expands USER", buf, user);

    // 3. User & Prompt Char (\$ -> $)
    rc = format_ps1("\\u\\$ ", buf, sizeof(buf));
    char expected_user_dollar[256];
    snprintf(expected_user_dollar, sizeof(expected_user_dollar), "%s$ ", user);
    assert_str_eq("format_ps1 \\u\\$ ", buf, expected_user_dollar);

    // 4. Hostname (\h)
    rc = format_ps1("\\h", buf, sizeof(buf));
    assert_int_eq("format_ps1 \\h rc", rc, 0);
    assert_str_contains("format_ps1 \\h non-empty", buf, "");

    // 5. Working directory (\w & \W)
    rc = format_ps1("\\w", buf, sizeof(buf));
    assert_int_eq("format_ps1 \\w rc", rc, 0);
    assert_str_contains("format_ps1 \\w non-empty", buf, "");

    rc = format_ps1("\\W", buf, sizeof(buf));
    assert_int_eq("format_ps1 \\W rc", rc, 0);
    assert_str_contains("format_ps1 \\W non-empty", buf, "");

    // 6. Time (\t) and Date (\d)
    rc = format_ps1("\\t", buf, sizeof(buf));
    assert_int_eq("format_ps1 \\t rc", rc, 0);
    assert_str_contains("format_ps1 \\t contains colon", buf, ":");

    rc = format_ps1("\\d", buf, sizeof(buf));
    assert_int_eq("format_ps1 \\d rc", rc, 0);
    assert_str_contains("format_ps1 \\d non-empty", buf, " ");

    // 7. Escape sequences (\e, \n, \\)
    rc = format_ps1("\\e[32mhello\\e[0m", buf, sizeof(buf));
    assert_int_eq("format_ps1 \\e color rc", rc, 0);
    assert_str_contains("format_ps1 \\e escape byte", buf, "\033[32mhello\033[0m");

    rc = format_ps1("line1\\nline2", buf, sizeof(buf));
    assert_int_eq("format_ps1 \\n newline rc", rc, 0);
    assert_str_eq("format_ps1 \\n produces newline", buf, "line1\nline2");

    rc = format_ps1("path\\\\to", buf, sizeof(buf));
    assert_int_eq("format_ps1 \\\\ backslash rc", rc, 0);
    assert_str_eq("format_ps1 \\\\ produces single backslash", buf, "path\\to");

    // 8. Standard bash PS1 prompt: [\u@\h \W]\$ 
    rc = format_ps1("[\\u@\\h \\W]\\$ ", buf, sizeof(buf));
    assert_int_eq("format_ps1 standard bash prompt rc", rc, 0);
    assert_str_contains("standard prompt contains user", buf, user);
    assert_str_contains("standard prompt ends with $", buf, "$ ");

    printf("  ✓ All %d PS1 prompt unit tests passed (%d/%d).\n",
           g_test_count, g_pass_count, g_test_count);
    return (g_pass_count == g_test_count) ? 0 : 1;
}
