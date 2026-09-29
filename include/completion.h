#ifndef APEX_COMPLETION_H
#define APEX_COMPLETION_H

#include <stddef.h>

/* Performs tab autocompletion on line buffer at cursor position */
void complete_word(char *buf, size_t *len, size_t *pos, size_t buf_max);

#endif /* APEX_COMPLETION_H */
