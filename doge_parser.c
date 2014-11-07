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
    mpc_parser_t* Adjective     = mpc_new("adjective");
    mpc_parser_t* Noun          = mpc_new("noun");
    mpc_parser_t* Phrase        = mpc_new("phrase");
    mpc_parser_t* Doge          = mpc_new("doge");

    /* Define them with the following language */
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                                                     \
            adjective   : /wow/ | /many/ | /so/ | /such/ ;                                    \
            noun        : /lisp/ | /language/ | /book/ | /build/ | /c/ ;                      \
            phrase      : <adjective> <noun> ;                                                \
            doge        : /^/ <phrase>* /$/ ;                                                 \
        ",
        Adjective, Noun, Phrase, Doge);

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
        if(mpc_parse("<stdin>", input, Doge, &r)) {
            /* on success print the AST */
            mpc_ast_print(r.output);
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
    mpc_cleanup(4, Adjective, Noun, Phrase, Doge);

    return 0;
}