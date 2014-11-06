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

int main(int argc, char** argv) {

    /* Create some parsers */
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("")

    /* Version and exit information for the user */
    puts("Lispy version 0.0.0.0.1");
    puts("Press Ctrl+c to exit\n");

    /* This is a loop that never ends */
    while(1) {

        /* Output the prompt and get input */
        char* input = readline("lispy> ");

        /* Add input to the history */
        add_history(input);

        /* Echo input back to the user */
        printf("No, you're a %s\n", input);

        /* Free retrieved input */
        free(input);        
    }

    return 0;
}