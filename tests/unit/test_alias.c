#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "alias.h"

#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define NC    "\033[0m"

static int g_tests = 0;
static int g_passed = 0;

static void assert_condition(const char *name, int condition) {
    g_tests++;
    if (condition) {
        g_passed++;
        printf("  Unit %2d: %-52s %s[PASS]%s\n", g_tests, name, GREEN, NC);
    } else {
        printf("  Unit %2d: %-52s %s[FAIL]%s\n", g_tests, name, RED, NC);
    }
}

int main(void) {
    printf("▶ Running C Unit Tests: alias table subsystem\n");

    alias_init();

    // 1. Initial lookup is NULL
    assert_condition("initial lookup of unset alias is NULL", alias_get("ll") == NULL);

    // 2. Set alias and get
    alias_set("ll", "ls -la");
    const char *val = alias_get("ll");
    assert_condition("alias_set stores value correctly", val != NULL && strcmp(val, "ls -la") == 0);

    // 3. Set another alias
    alias_set("gco", "git checkout");
    const char *gco = alias_get("gco");
    assert_condition("stores multiple distinct aliases", gco != NULL && strcmp(gco, "git checkout") == 0);

    // 4. Overwrite alias
    alias_set("ll", "ls -lh");
    val = alias_get("ll");
    assert_condition("overwriting alias updates value", val != NULL && strcmp(val, "ls -lh") == 0);

    // 5. Unset alias
    int rc = alias_unset("ll");
    assert_condition("alias_unset returns 0 on success", rc == 0);
    assert_condition("alias_unset removes alias from table", alias_get("ll") == NULL);

    // 6. Unset non-existent alias
    rc = alias_unset("nonexistent_alias_xyz");
    assert_condition("alias_unset returns non-zero for missing alias", rc != 0);

    // 7. Cleanup
    alias_cleanup();
    assert_condition("after cleanup alias_get is NULL", alias_get("gco") == NULL);

    if (g_passed == g_tests) {
        printf("  %s✓ All %d alias unit tests passed.%s\n", GREEN, g_tests, NC);
        return 0;
    }
    printf("  %s✗ %d/%d alias unit tests failed.%s\n", RED, g_tests - g_passed, g_tests, NC);
    return 1;
}
