#include <ctype.h>

#include "symbol.h"
#include "pair.h"

#include "baby_reader.h"


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
        }
    }
    
    symbol[i] = '\0';

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
        // TODO: Read CDR
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
    } else {
        ungetc(c, f);
        return read_symbol(f, fail);
    }
}
