#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

/* If we are compiling on windows then do this */
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* Fake readline function */
char* readline(char* prompt) {
    fputs(prompt, stdout);
    fgets(buffer, 2048, stdin);
    char* cpy = malloc(strlen(buffer)+1);
    strcpy(cpy, buffer);
    cpy[strlen(cpy)-1] = '\0';
    return cpy; 
}

/* Fake add_history function */
void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else
#include <editline/readline.h>
#endif

/* create enumeration of possible lval types */
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };
/* creare enumeration of possible error types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* declare new lval struct */
typedef struct lval {
    int type;
    long num;
    /* error and symbol types have some string data */
    char* err;
    char* sym;
    /* count and pointer to a list of 'lval*' */
    int count;
    struct lval** cell;
} lval;

/* create new number type lval */
lval* lval_num(long x) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_NUM;
    v->num = x;
    return v;
}

/* create a new error type lval */
lval* lval_err(char* m) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_ERR;
    v->err = malloc(strlen(m) + 1); /* the +1 accounts for the null terminator at the end of each string */
    strcpy(v->err, m);
    return v;
}

lval* lval_sym(char* s) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SYM;
    v->sym = malloc(sizeof(s) + 1);
    strcpy(v->sym, s);
    return v;
}

lval* lval_sexpr(void) {
    lval* v = malloc(sizeof(lval));
    v->type = LVAL_SEXPR;
    v->count = 0;
    v->cell = NULL;
    return v;
}

void lval_del(lval* v) {

    switch(v->type) {

        /* do nothing special when it's the number type */
        case LVAL_NUM: break;

        /* for Err or Sym free the string data */
        case LVAL_ERR: free(v->err); break;
        case LVAL_SYM: free(v->sym); break;

        /* if Sexpr then delete all of the elements inside */
        case LVAL_SEXPR:
            for(int i = 0; i < v->count; i++) {
                lval_del(v->cell[i]);
            }
            /* also free the memory allocated to contain the pointers */
            free(v->cell);
        break;
    }

    /* free the memory allocated for the "lval" struct itself */
    free(v);
}

lval* lval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ?
        lval_num(x) : lval_err("invalid number");
}

lval* lval_add(lval* v, lval* x) {
    v->count++;
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    v->cell[v->count-1] = x;
    return v;
}

lval* lval_read(mpc_ast_t* t) {

    /* if symbol or number return conversion to that type */
    if(strstr(t->tag, "number")) { return lval_read_num(t); }
    if(strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

    /* if root (>) or sexpr then create empty list */
    lval* x = NULL;
    if(strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
    if(strcmp(t->tag, "sexpr"))  { x = lval_sexpr(); }

    for(int i = 0; i < t->children_num; i++) {
        if(strcmp(t->children[i]->contents, "(") == 0) { continue; }
        if(strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if(strcmp(t->children[i]->contents, "{") == 0) { continue; }
        if(strcmp(t->children[i]->contents, ")") == 0) { continue; }
        if(strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
        x = lval_add(x, lval_read(t->children[i]));
    }

    return x;
}

void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
    putchar(open);
    for(int i = 0; i < v->count; i++) {

        /* prints value combined within */
        lval_print(v->cell[i]);

        /* don't print the trailing space if it's the last element */
        if(i != (v->count-1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

/* print an lval */
void lval_print(lval* v) {
    switch(v->type) {
        case LVAL_NUM: printf("%li", v->num);           break;
        case LVAL_ERR: printf("Error: %s", v->err);     break;
        case LVAL_SYM: printf("%s", v->sym);            break;
        case LVAL_SEXPR: lval_expr_print(v, '(', ')');  break;            
    }
}

/* prints an lval with a new line appended */
void lval_println(lval* v) {
    lval_print(v); putchar('\n');
}

lval* lval_pop(lval* v, int i) {

    /* find the item at i */
    lval* x = v->cell[i];

    /* shift memory after the item "i" over the top */
    memmove(&v->cell[i], &v->cell[i+1],
        sizeof(lval*) * (v->count-i-1));

    /* decrease the count of items in the list */
    v->count--;

    /* reallocate the memory used */
    v->cell = realloc(v->cell, sizeof(lval*) * v->count);
    return x;
}

lval* lval_take(lval* v, int i) {
    lval* x = lval_pop(v, i);
    lval_del(v);
    return x;
}

lval* builtin_op(lval* a, char* op) {

    /* ensure all the arguments are numbers */
    for( int i = 0; i < a->count; i++) {
        if(a-> cell[i]->type != LVAL_NUM) {
            lval_del(a);
            return lval_err("Cannot operate on a non-number");
        }
    }

    /* pop the first element */
    lval* x = lval_pop(a, 0);

    /* if no arguments and sub then perform unary negation */
    if((strcmp(op, "-") == 0) && a->count == 0) {
        x->num = -x->num;
    }

    /* while there are still elements remaining */
    while(a->count > 0) {

        /* pop the next element */
        lval* y = lval_pop(a, 0);

        if(strcmp(op, "+") == 0) { x->num += y->num; }
        if(strcmp(op, "-") == 0) { x->num -= y->num; }
        if(strcmp(op, "*") == 0) { x->num *= y->num; }
        if(strcmp(op, "/") == 0) { 
            if(y->num == 0) {
                lval_del(x); lval_del(y);
                x = lval_err("Division By Zero!"); break;
            }
            x->num /= y->num;
        }

        lval_del(y);
    }

    lval_del(a); return x;
}

lval* lval_eval(lval* v);

lval* lval_eval_sexpr(lval* v) {

    /* evaluate children */
    for (int i = 0; i < v->count; i++) {
        v->cell[i] = lval_eval(v->cell[i]);
    }

    /* error checking */
    for (int i = 0; i < v->count; i++) {
        if(v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
    }

    /* empty expression */
    if(v->count == 0) { return v;}

    /* single expression */
    if(v->count == 1) { return lval_take(v, 0); }

    /* ensure first element is a symbol */
    lval* f = lval_pop(v, 0);
    if(f->type != LVAL_SYM) {
        lval_del(f); lval_del(v);
        return lval_err("S-expression Does not start with a symbol");
    }

    /* call builtin with operator */
    lval* result = builtin_op(v, f->sym);
    lval_del(f);
    return result;
}

lval* lval_eval(lval* v) {
    /* evaluate sexpressions */
    if(v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }
    /* all other lval types remain the same */
    return v;
}

int main(int argc, char** argv) {

    /* Create some parsers */
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Symbol    = mpc_new("symbol");
    mpc_parser_t* Sexpr     = mpc_new("sexpr");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Lispy     = mpc_new("lispy");

    /* Define them with the following language */
    mpca_lang(MPCA_LANG_DEFAULT,
        "\
            number  : /-?[0-9]+/ ;\
            symbol  : '+' | '-' | '*' | '/' ;\
            sexpr   : '(' <expr>* ')' ;\
            expr    : <number> | <symbol> | <sexpr> ;\
            lispy   : /^/ <expr>* /$/ ;\
        ",
        Number, Symbol, Sexpr, Expr, Lispy);

    /* Version and exit information for the user */
    puts("Lispy version 0.0.0.0.1");
    puts("Press Ctrl+c to exit\n");

    /* This is a loop that never ends */
    while(1) {

        /* Output the prompt and get input */
        char* input = readline("lispy> ");

        /* Add input to the history */
        add_history(input);

        /* Attempt to Parse the user Input */
        mpc_result_t r;
        if(mpc_parse("<stdin>", input, Lispy, &r)) {

            lval* x = lval_eval(lval_read(r.output));
            lval_println(x);
            lval_del(x);
        } else {

            /* Otherwise print the error */
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        /* Free retrieved input */
        free(input);        
    }

    /* Undefine and Delete the Parsers */
    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);

    return 0;
}