#ifndef APEX_HIGHLIGHT_H
#define APEX_HIGHLIGHT_H

#include <stddef.h>

/* Renders input buffer to stdout with ANSI syntax highlighting */
void print_syntax_highlighted(const char *buf, size_t len);

#endif /* APEX_HIGHLIGHT_H */
