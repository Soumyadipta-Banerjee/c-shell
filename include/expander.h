#ifndef APEX_EXPANDER_H
#define APEX_EXPANDER_H

#include "parser.h"

/* Expands variables ($?, $$, $VAR, ${VAR}), command substitutions ($(cmd), `cmd`),
   arithmetic expansions ($(( expr ))), and tildes (~) for a shell token */
char *expand_token(const ShellToken *tok);

#endif /* APEX_EXPANDER_H */
