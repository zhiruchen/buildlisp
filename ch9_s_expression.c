#include<stdio.h>
#include<stdlib.h>
#include"mpc.h"

#include<editline/readline.h>

//lval Lisp Value
typedef struct {
  int type;  //Number or Error
  long num;  //value
  char* err; //error string
  char* sym;
  /* Count and Pointer to a list of "lval*" */
  int count;
  struct lval** cell;
} lval;

/* 创建lval的可能的值的枚举类型 */
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

/* 创建一个指向lval Number的指针 */
lval* lval_num(long x) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_NUM;
  v->num = x;
  return v;
}

/* 构造一个指向新的Error lval 的指针*/
lval* lval_err(char* m) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_ERR;
  v->err = malloc(strlen(m) + 1);
  strcpy(v->err, m);
  return v;
}

/* 创建一个指向新Symbol lval 的指针 */
lval* lval_sym(char* s) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SYM;
  v->sym = malloc(strlen(s) + 1);
  strcpy(v->sym, s);
  return v;
}

/* 一个指向新的S表达式的lval的指针 */
lval* lval_sexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_SEXPR;
  v->count = 0;
  v->cell = NULL; // NUll is a special contant that points to memory location 0
  return v;
}

void lval_del(lval* v) {
  switch (v->type) {
    case LVAL_NUM: break;
    /* 对错误和符号来说，删除对应的字符串*/
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;

    /* 如果是sexpr 则删除其中所有元素*/
    case LVAL_SEXPR:
      for (int i = 0; i < v->count; i++) {
        lval_del(v->cell[i]);
      }
      free(v->cell);
    break;
  }
  /* 释放lval结构体本身被分配的内存*/
  free(v);
}

lval* lval_read_num(mpc_ast_t* t) {
  errno = 0;
  long x = strtol(t->contents, NULL, 10);
  return errno != ERANGE ?
    lval_num(x): lval_err("invalid number");
}


lval* lval_add(lval* v, lval* x) {
  v->count++;  //子表达式计数加1
  /* 重新分配内存 */
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  v->cell[v->count-1] = x;  //存入子表达式x
  return v;
}
lval* lval_read(mpc_ast_t* t) {

  /* if symbol or number return conversion to that type */
  if (strstr(t->tag, "number")) { return lval_read_num(t); }
  if (strstr(t->tag, "symbol")) { return lval_sym(t->contents); }

  /* If root (>) or sexpr then create empty list */
  lval* x = NULL;
  if (strcmp(t->tag, ">") == 0) { x = lval_sexpr(); }
  if (strstr(t->tag, "sexpr"))  { x = lval_sexpr(); }

  /* Fill this list with any valid expression contained within */
  for (int i = 0; i < t->children_num; i++) {
    if (strcmp(t->children[i]->contents, "(") == 0) { continue; }
    if (strcmp(t->children[i]->contents, ")") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "{") == 0) { continue; }
    if (strcmp(t->children[i]->contents, "}") == 0) { continue; }
    if (strcmp(t->children[i]->tag,  "regex") == 0) { continue; }
    x = lval_add(x, lval_read(t->children[i]));
  }

  return x;

}

void lval_print(lval* v);

void lval_expr_print(lval* v, char open, char close) {
  putchar(open);
  for (int i = 0; i < v->count; i++) {
    lval_print(v->cell[i]);
    if (i != v->count-1) {
      putchar(' ');
    }
  }
  putchar(close);
}

void lval_print(lval* v) {
  switch (v->type) {
    case LVAL_NUM: printf("%li", v->num); break;
    case LVAL_ERR: printf("%s",  v->err); break;
    case LVAL_SYM: printf("%s",  v->sym); break;
    case LVAL_SEXPR: lval_expr_print(v, '(', ')'); break;
  }
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }


// lval eval(mpc_ast_t* t){
//
//   /*如果被标示为数字则直接返回*/
//   if (strstr(t->tag, "number")) {
//     errno = 0;
//     long x = strtol(t->contents, NULL, 10);
//     return errno != ERANGE ? lval_num(x): lval_err(LERR_BAD_NUM);
//   }
//
//   /*如果一个节点被标记为expr但不是数字，那么需要检查它的第二个child是什么操作符*/
//   char* op = t->children[1]->contents;
//
//   /*将第三个child存入x*/
//   lval x = eval(t->children[2]);
//
//   int i = 3;
//   while (strstr(t->children[i]->tag, "expr")) {
//     /* code */
//     x = eval_op(x, op, eval(t->children[i]));
//     i++;
//   }
//
//   return x;
// }


// long eval(mpc_ast_t* t){
//
//   /*如果被标示为数字则直接返回*/
//   if (strstr(t->tag, "number")) {
//     return atoi(t->contents);
//   }
//
//   /*如果一个节点被标记为expr但不是数字，那么需要检查它的第二个child是什么操作符*/
//   char* op = t->children[1]->contents;
//
//   /*将第三个child存入x*/
//   long x = eval(t->children[2]);
//
//   int i = 3;
//   while (strstr(t->children[i]->tag, "expr")) {
//     /* code */
//     x = eval_op(x, op, eval(t->children[i]));
//     i++;
//   }
//
//   return x;
// }

int main(int argc, char const *argv[]) {
  /* polish notation */
  // create some parser
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  /* define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                   \
    number   :  /-?[0-9]+/ ;                            \
    symbol   :  '+' | '-' | '*' | '/' ;                 \
    sexpr    :  '(' <expr>* ')' ;                       \
    expr     :  <number> | <symbol> | <sexpr>; \
    lispy    :  /^/ <expr>* /$/ ;            \
    ",
    Number, Symbol, Sexpr, Expr, Lispy);

  puts("Lispy Version 0.0.0.0.1");
  puts("Please press Ctrl + C to Exit\n");

  while (1) {
    /* output our prompt and get the input*/
    char* input = readline("lispy--> ");
    /* Add input to history*/
    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)){

      lval* v = lval_read(r.output);
      lval_println(v);
      lval_del(v);

    }
    else{
      /* otherwise print error*/
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Lispy);  //undefine and delete parsers
  return 0;
}
