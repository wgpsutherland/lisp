lisp
====

My own implementation of lisp, following the tutorial [here](http://www.buildyourownlisp.com/).

To run parsing.c:
    
    cc -std=c99 -Wall parsing.c mpc.c -ledit -lm -o parsing

    ./ parsing

To run doge_parse.c:
    
    cc -std=c99 -Wall doge_parser.c mpc.c -ledit -lm -o doge_parser

    ./ doge_parser