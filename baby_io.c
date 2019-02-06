#include <ctype.h>
#include <string.h>

#include "symbol.h"
#include "pair.h"
#include "vector.h"
#include "character.h"
#include "closure.h"

#include "baby_io.h"



static bool is_whitespace_char(int c)
{
    return (c == ' ') || (c == '\t') || (c == '\n');
}


static bool is_digit(int c)
{
    return isdigit(c);
}



static void slurp_whitespace(FILE *f)
{
    int c;

    do {
        c = fgetc(f);
    } while(is_whitespace_char(c));
    
    ungetc(c, f);
}


static objptr_t read_symbol(FILE *f, bool *fail)
{
    int c;
    unsigned int i;
    char symbol[64];

    for (i = 0; i < sizeof(symbol) - 1; i++)
    {
        c = fgetc(f);
        if (c == EOF || is_whitespace_char(c) || c == ')') {
            ungetc(c, f);
            break;
        } else {
            symbol[i] = c;
        }
    }
    
    symbol[i] = '\0';

    if (strcmp(symbol, "#t") == 0) {
        return NIL_TRUE;
    } else if (strcmp(symbol, "#f") == 0) {
        return NIL_FALSE;
    }
    
    *fail = (i == 0);
    return c_string_to_symbol(symbol);
}


static objptr_t read_list(FILE *f, bool *fail)
{
    unsigned int c;
    objptr_t obj;

    slurp_whitespace(f);
    c = fgetc(f);

    if (c == ')') {
        return EMPTY_LIST;
    } else if (c == '.') {
        obj = baby_read(f, fail);
        if (*fail) return EMPTY_LIST;
        slurp_whitespace(f);
        c = fgetc(f);
        *fail = !(c == ')');
        return obj;
    } else {
        ungetc(c, f);
        obj = baby_read(f, fail);
        if (*fail) return EMPTY_LIST;
        return cons(obj, read_list(f, fail));
    }
}


objptr_t baby_read(FILE *f, bool *fail)
{
    int c;

    *fail = false;
    
    slurp_whitespace(f);
    c = fgetc(f);

    if (c == '(') {
        return read_list(f, fail);
    } else if (is_digit(c)) {
        // TODO
        return EMPTY_LIST;
    } else if (c == '\'') {
        return cons(SYMBOL_QUOTE, cons(baby_read(f, fail), EMPTY_LIST));
    } else {
        ungetc(c, f);
        return read_symbol(f, fail);
    }
}

void baby_print(objptr_t expr)
{
    if (is_of_type(expr, &TYPE_PAIR)) {
        putchar('(');
        baby_print(get_car(expr));
        printf(" . ");
        baby_print(get_cdr(expr));
        putchar(')');
    } else if (is_of_type(expr, &TYPE_SYMBOL)) {
        struct symbol *symb = (struct symbol*) dereference(expr);
        for (unsigned int i = 0; i < vector_length(symb->name_string); i++) {
            putchar(character_value(vector_get(symb->name_string, i)));
        }
    } else if (is_of_type(expr, &TYPE_CLOSURE)) {
        printf("#<closure:%x>", expr);
    } else if (is_of_type(expr, &TYPE_CLOSURE_PROTOTYPE)) {
        printf("#<closure-prototype:%x>", expr);
    } else if (is_of_type(expr, &TYPE_CHARACTER)) {
        printf("#<character:%x>", expr);
    } else if (is_of_type(expr, &TYPE_VECTOR)) {
        printf("#<vector:%x>", expr);
    } else if (expr == EMPTY_LIST) {
        printf("()");
    } else if (expr == NIL_TRUE) {
        printf("#t");
    } else if (expr == NIL_FALSE) {
        printf("#f");
    } else {
        printf("???");
    }
}
