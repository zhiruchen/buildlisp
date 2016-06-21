#include<stdio.h>
#include<stdlib.h>
#include"mpc.h"

#include<editline/readline.h>


//lval Lisp Value
typedef struct {
  int type;  //Number or Error
  long num;  //value
  int err; //error type
} lval;

/* 创建lval的可能的值的枚举类型 */
enum { LVAL_NUM, LVAL_ERR };

/**/
enum { LERR_DIV_ZERO, LERR_BAD_OP, LERR_BAD_NUM };


/* 将长整型数装配为 lval*/
lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

/*Print an "lval "*/
void lval_print(lval v) {
  switch (v.type) {
    /* 如果是数字，直接输出 */
    case LVAL_NUM: printf("%li", v.num); break;
    case LVAL_ERR:
      /* 检查是什么错误类型，然后好输出 */
      if (v.err == LERR_DIV_ZERO) {
        printf("Error: Division By Zero!");
      }
      if (v.err == LERR_BAD_OP) {
        printf("Error: Invalid Operator!");
      }
      if (v.err == LERR_BAD_NUM) {
        printf("Error: Invalid Number!");
      }
      break;
  }
}

void lval_println(lval v) { lval_print(v); putchar('\n'); }


/*检查操作符，决定采取什么操作*/
lval eval_op(lval x, char* op, lval y) {
  /* 两个操作数当中的任何一个有错误就返回 */
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  /*计算*/
  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) {
    return y.num == 0
      ? lval_err(LERR_DIV_ZERO)
      : lval_num(x.num / y.num);
  }
  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t){

  /*如果被标示为数字则直接返回*/
  if (strstr(t->tag, "number")) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x): lval_err(LERR_BAD_NUM);
  }

  /*如果一个节点被标记为expr但不是数字，那么需要检查它的第二个child是什么操作符*/
  char* op = t->children[1]->contents;

  /*将第三个child存入x*/
  lval x = eval(t->children[2]);

  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    /* code */
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
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
      lval result = eval(r.output);
      lval_println(result);
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
