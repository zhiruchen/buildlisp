#include<stdio.h>
#include<stdlib.h>
#include"mpc.h"

#include<editline/readline.h>

/*检查操作符，决定采取什么操作*/
long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  return 0;
}

long eval(mpc_ast_t* t){

  /*如果被标示为数字则直接返回*/
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  /*如果一个节点被标记为expr但不是数字，那么需要检查它的第二个child是什么操作符*/
  char* op = t->children[1]->contents;

  /*将第三个child存入x*/
  long x = eval(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    /* code */
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

int number_of_leaves(mpc_ast_t* t){
  if (t->children_num == 0) { return 1; }
  if (t->children_num >= 1) {
    int total = 1;
    for(int i=0; i<t->children_num; i++) {
      total = total + number_of_leaves(t->children[i]);
    }
    return total;
  }

  return 0;
}

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
      long result = eval(r.output);
      printf("%li\n", result);
      printf("叶子结点个数: %d\n", number_of_leaves(r.output));
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
