#include<stdio.h>
#include<stdlib.h>
#include"mpc.h"

#include<editline/readline.h>


int main(int argc, char const *argv[]) {
  /* polish notation */
  // create some parser
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  /* define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                   \
    number   :  /-?[0-9]+/ ;                            \
    operator :  '+' | '-' | '*' | '/' ;                 \
    expr     :  <number> | '(' <operator> <expr>+ ')' ; \
    lispy    :  /^/ <operator> <expr>+ /$/ ;            \
    ",
    Number, Operator, Expr, Lispy);

  puts("Lispy Version 0.0.0.0.1");
  puts("Please press Ctrl + C to Exit\n");

  while (1) {
    /* output our prompt and get the input*/
    char* input = readline("lispy--> ");
    /* Add input to history*/
    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)){
      /* On success print the AST*/
      mpc_ast_print(r.output);
      mpc_ast_delete(r.output);
    }
    else{
      /* otherwise print error*/
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(4, Number, Operator, Expr, Lispy);  //undefine and delete parsers
  return 0;
}
