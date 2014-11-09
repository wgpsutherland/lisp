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
enum { LVAL_NUM, LVAL_ERR };
/* creare enumeration of possible error types */
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };

/* declare new lval struct */
typedef struct {
    int type;
    long num;
    int err;
} lval;

/* create new number type lval */
lval lval_num(long x) {
    lval v;
    v.type = LVAL_NUM;
    v.num = x;
    return v;
}

/* create a new error type lval */
lval lval_err(int x) {
    lval v;
    v.type = LVAL_ERR;
    v.err = x;
    return v;
}

/* print an lval */
void lval_print(lval v) {
    switch(v.type) {
        
        /* if the type is a number then print it and break out of the switch */
        case LVAL_NUM: printf("%li", v.num); break;

        /* if the type is an error instead */
        case LVAL_ERR: 
            /* check to see what type of error it is and print it */
            if(v.err == LERR_DIV_ZERO) {
                printf("Error: divide by zero.");
            }
            if(v.err == LERR_BAD_OP) {
                printf("Error: invalid operator.");
            }
            if(v.err == LERR_BAD_NUM) {
                printf("Error: invalid number.");
            }
        break;
    }
}

void lval_println(lval v) {
    lval_print(v); putchar('\n');
}

lval eval_op(lval x, char* op, lval y) {

    /* if either value is an error then return it */
    if (x.type == LVAL_ERR) { return x; }
    if (y.type == LVAL_ERR) { return y; }

    if(strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
    if(strcmp(op, "-") == 0) { 
        if(y.num) { /* if there are two arguments then do standard subtraction */
            return lval_num(x.num - y.num);
        } else { /* if there is only one argument then return the negative of the number */
            return lval_num(x.num * -1);
        }
    }
    if(strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
    if(strcmp(op, "/") == 0) { 
        return y.num == 0 /* prevents division by 0 so the program doesn't crash */
            ? lval_err(LERR_DIV_ZERO)
            : lval_num(x.num / y.num);
    }
    if(strcmp(op, "%") == 0) { return lval_num(x.num % y.num); }
    if(strcmp(op, "^") == 0) { return lval_num(pow(x.num,y.num)); }
    if(strcmp(op, "min") == 0) { /* finds the minimum of the given arguments */
        if(x.num <= y.num) {
            return lval_num(x.num);
        } else {
            return lval_num(y.num);
        }
    }
    if(strcmp(op, "max") == 0) { /* finds the maximum of the given arguments */
        if(x.num >= y.num) {
            return lval_num(x.num);
        } else {
            return lval_num(y.num);
        }
    }
    return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {

    /* If tagged as number return it directly */
    if (strstr(t->tag, "number")) {
        /* check if there is an error in the conversion */
        errno = 0;
        long x = strtol(t->contents, NULL, 10);
        return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
    }

    /* The operator is always the second child */
    char* op = t->children[1]->contents;

    /* We store the third child in 'x' */
    lval x = eval(t->children[2]);

    /* Iterate the remaining children and combining */
    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;
}

int main(int argc, char** argv) {

    /* Create some parsers */
    mpc_parser_t* Number    = mpc_new("number");
    mpc_parser_t* Operator  = mpc_new("operator");
    mpc_parser_t* Expr      = mpc_new("expr");
    mpc_parser_t* Lispy     = mpc_new("lispy");

    /* Define them with the following language */
    mpca_lang(MPCA_LANG_DEFAULT,
        "\
            number  : /-?[0-9]+/ ;\
            operator: '+' | '-' | '*' | '/' | '%' | '^' | /min/ | /max/ ;\
            expr     : <number> | '(' <operator> <expr>+ ')' ;\
            lispy    : /^/ <operator> <expr>+ /$/ ;\
        ",
        Number, Operator, Expr, Lispy);

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
            /* on success print the AST */
            lval result = eval(r.output);
            lval_println(result);
            mpc_ast_delete(r.output);
        } else {
            /* Otherwise print the error */
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        /* Free retrieved input */
        free(input);        
    }

    /* Undefine and Delete the Parsers */
    mpc_cleanup(4, Number, Operator, Expr, Lispy);

    return 0;
}