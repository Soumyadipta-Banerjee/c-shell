#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "arithmetic.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct {
    const char *src;
    size_t pos;
    int has_error;
} MathParser;

static void skip_ws(MathParser *p)
{
    while (p->src[p->pos] && isspace((unsigned char)p->src[p->pos]))
    {
        p->pos++;
    }
}

static long long parse_expr(MathParser *p);

static long long parse_primary(MathParser *p)
{
    skip_ws(p);
    char c = p->src[p->pos];

    if (c == '\0')
    {
        p->has_error = 1;
        return 0;
    }

    if (c == '(')
    {
        p->pos++; // consume '('
        long long val = parse_expr(p);
        skip_ws(p);
        if (p->src[p->pos] == ')')
        {
            p->pos++; // consume ')'
        }
        else
        {
            p->has_error = 1;
        }
        return val;
    }

    if (isdigit((unsigned char)c))
    {
        char *endptr = NULL;
        long long val = strtoll(&p->src[p->pos], &endptr, 0);
        p->pos = (size_t)(endptr - p->src);
        return val;
    }

    if (c == '$' || isalpha((unsigned char)c) || c == '_')
    {
        if (c == '$') p->pos++;
        size_t start = p->pos;
        while (isalnum((unsigned char)p->src[p->pos]) || p->src[p->pos] == '_')
        {
            p->pos++;
        }
        size_t len = p->pos - start;
        if (len == 0)
        {
            p->has_error = 1;
            return 0;
        }
        char varname[128];
        if (len >= sizeof(varname)) len = sizeof(varname) - 1;
        strncpy(varname, &p->src[start], len);
        varname[len] = '\0';

        const char *val_str = getenv(varname);
        if (!val_str || val_str[0] == '\0')
        {
            return 0;
        }
        return strtoll(val_str, NULL, 0);
    }

    p->has_error = 1;
    return 0;
}

static long long parse_unary(MathParser *p)
{
    skip_ws(p);
    char c = p->src[p->pos];
    if (c == '+')
    {
        p->pos++;
        return parse_unary(p);
    }
    if (c == '-')
    {
        p->pos++;
        return -parse_unary(p);
    }
    if (c == '!')
    {
        p->pos++;
        return !parse_unary(p);
    }
    if (c == '~')
    {
        p->pos++;
        return ~parse_unary(p);
    }
    return parse_primary(p);
}

static long long parse_mul_div(MathParser *p)
{
    long long left = parse_unary(p);
    while (!p->has_error)
    {
        skip_ws(p);
        char c = p->src[p->pos];
        if (c == '*' && p->src[p->pos + 1] != '*')
        {
            p->pos++;
            long long right = parse_unary(p);
            left = left * right;
        }
        else if (c == '/')
        {
            p->pos++;
            long long right = parse_unary(p);
            if (right == 0)
            {
                fprintf(stderr, "apex-shell: arithmetic: division by 0\n");
                p->has_error = 1;
                return 0;
            }
            left = left / right;
        }
        else if (c == '%')
        {
            p->pos++;
            long long right = parse_unary(p);
            if (right == 0)
            {
                fprintf(stderr, "apex-shell: arithmetic: modulo by 0\n");
                p->has_error = 1;
                return 0;
            }
            left = left % right;
        }
        else
        {
            break;
        }
    }
    return left;
}

static long long parse_add_sub(MathParser *p)
{
    long long left = parse_mul_div(p);
    while (!p->has_error)
    {
        skip_ws(p);
        char c = p->src[p->pos];
        if (c == '+' && p->src[p->pos + 1] != '+')
        {
            p->pos++;
            long long right = parse_mul_div(p);
            left = left + right;
        }
        else if (c == '-' && p->src[p->pos + 1] != '-')
        {
            p->pos++;
            long long right = parse_mul_div(p);
            left = left - right;
        }
        else
        {
            break;
        }
    }
    return left;
}

static long long parse_shift(MathParser *p)
{
    long long left = parse_add_sub(p);
    while (!p->has_error)
    {
        skip_ws(p);
        if (p->src[p->pos] == '<' && p->src[p->pos + 1] == '<')
        {
            p->pos += 2;
            long long right = parse_add_sub(p);
            if (right < 0 || right >= 64)
            {
                left = 0;
            }
            else
            {
                left = (long long)((unsigned long long)left << right);
            }
        }
        else if (p->src[p->pos] == '>' && p->src[p->pos + 1] == '>')
        {
            p->pos += 2;
            long long right = parse_add_sub(p);
            if (right < 0 || right >= 64)
            {
                left = 0;
            }
            else
            {
                left = left >> right;
            }
        }
        else
        {
            break;
        }
    }
    return left;
}

static long long parse_relational(MathParser *p)
{
    long long left = parse_shift(p);
    while (!p->has_error)
    {
        skip_ws(p);
        if (p->src[p->pos] == '<' && p->src[p->pos + 1] == '=')
        {
            p->pos += 2;
            long long right = parse_shift(p);
            left = (left <= right);
        }
        else if (p->src[p->pos] == '>' && p->src[p->pos + 1] == '=')
        {
            p->pos += 2;
            long long right = parse_shift(p);
            left = (left >= right);
        }
        else if (p->src[p->pos] == '<' && p->src[p->pos + 1] != '<')
        {
            p->pos += 1;
            long long right = parse_shift(p);
            left = (left < right);
        }
        else if (p->src[p->pos] == '>' && p->src[p->pos + 1] != '>')
        {
            p->pos += 1;
            long long right = parse_shift(p);
            left = (left > right);
        }
        else
        {
            break;
        }
    }
    return left;
}

static long long parse_equality(MathParser *p)
{
    long long left = parse_relational(p);
    while (!p->has_error)
    {
        skip_ws(p);
        if (p->src[p->pos] == '=' && p->src[p->pos + 1] == '=')
        {
            p->pos += 2;
            long long right = parse_relational(p);
            left = (left == right);
        }
        else if (p->src[p->pos] == '!' && p->src[p->pos + 1] == '=')
        {
            p->pos += 2;
            long long right = parse_relational(p);
            left = (left != right);
        }
        else
        {
            break;
        }
    }
    return left;
}

static long long parse_bitwise_and(MathParser *p)
{
    long long left = parse_equality(p);
    while (!p->has_error)
    {
        skip_ws(p);
        if (p->src[p->pos] == '&' && p->src[p->pos + 1] != '&')
        {
            p->pos++;
            long long right = parse_equality(p);
            left = left & right;
        }
        else
        {
            break;
        }
    }
    return left;
}

static long long parse_bitwise_xor(MathParser *p)
{
    long long left = parse_bitwise_and(p);
    while (!p->has_error)
    {
        skip_ws(p);
        if (p->src[p->pos] == '^')
        {
            p->pos++;
            long long right = parse_bitwise_and(p);
            left = left ^ right;
        }
        else
        {
            break;
        }
    }
    return left;
}

static long long parse_bitwise_or(MathParser *p)
{
    long long left = parse_bitwise_xor(p);
    while (!p->has_error)
    {
        skip_ws(p);
        if (p->src[p->pos] == '|' && p->src[p->pos + 1] != '|')
        {
            p->pos++;
            long long right = parse_bitwise_xor(p);
            left = left | right;
        }
        else
        {
            break;
        }
    }
    return left;
}

static long long parse_logical_and(MathParser *p)
{
    long long left = parse_bitwise_or(p);
    while (!p->has_error)
    {
        skip_ws(p);
        if (p->src[p->pos] == '&' && p->src[p->pos + 1] == '&')
        {
            p->pos += 2;
            long long right = parse_bitwise_or(p);
            left = (left && right);
        }
        else
        {
            break;
        }
    }
    return left;
}

static long long parse_logical_or(MathParser *p)
{
    long long left = parse_logical_and(p);
    while (!p->has_error)
    {
        skip_ws(p);
        if (p->src[p->pos] == '|' && p->src[p->pos + 1] == '|')
        {
            p->pos += 2;
            long long right = parse_logical_and(p);
            left = (left || right);
        }
        else
        {
            break;
        }
    }
    return left;
}

static long long parse_ternary(MathParser *p)
{
    long long cond = parse_logical_or(p);
    skip_ws(p);
    if (p->src[p->pos] == '?')
    {
        p->pos++; // consume '?'
        long long true_val = parse_expr(p);
        skip_ws(p);
        if (p->src[p->pos] == ':')
        {
            p->pos++; // consume ':'
            long long false_val = parse_ternary(p);
            return cond ? true_val : false_val;
        }
        else
        {
            p->has_error = 1;
            return 0;
        }
    }
    return cond;
}

static long long parse_expr(MathParser *p)
{
    return parse_ternary(p);
}

long long evaluate_arithmetic_expression(const char *expr, int *error)
{
    if (!expr)
    {
        if (error) *error = 1;
        return 0;
    }

    MathParser parser;
    parser.src = expr;
    parser.pos = 0;
    parser.has_error = 0;

    long long result = parse_expr(&parser);
    skip_ws(&parser);

    if (parser.src[parser.pos] != '\0')
    {
        parser.has_error = 1;
    }

    if (error)
    {
        *error = parser.has_error;
    }
    return parser.has_error ? 0 : result;
}
