#ifndef APEX_ARITHMETIC_H
#define APEX_ARITHMETIC_H

/* Evaluates an arithmetic expression string $(( expr )).
   Returns the evaluated value as a long long.
   If error is non-null, sets *error to 1 on syntax or division by zero error, or 0 on success. */
long long evaluate_arithmetic_expression(const char *expr, int *error);

#endif /* APEX_ARITHMETIC_H */
