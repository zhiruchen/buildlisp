#include<stdio.h>
#include<stdlib.h>
#include"mpc.h"

#include<editline/readline.h>

//lval Lisp Value
typedef struct lval{
  int type;  //Number or Error
  long num;  //value
  char* err; //error string
  char* sym;
  /* Count and Pointer to a list of "lval*" */
  int count;
  struct lval** cell;
} lval;

/* 创建lval的可能的值的枚举类型 */
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR };

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

lval* lval_qexpr(void) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_QEXPR;
  v->count = 0;
  v->cell = NULL;
  return v;
}

void lval_del(lval* v) {
  switch (v->type) {
    case LVAL_NUM: break;
    /* 对错误和符号来说，删除对应的字符串*/
    case LVAL_ERR: free(v->err); break;
    case LVAL_SYM: free(v->sym); break;

    /* 如果是qexpr, sexpr 则删除其中所有元素*/
    case LVAL_QEXPR:
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
  if (strstr(t->tag, "qexpr"))  { x = lval_qexpr(); }

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

lval* lval_pop(lval* v, int i) {
  /* 找到要弹出的lval* */
  lval* x = v->cell[i];

  //Shift the memory after the item at i over the top
  memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count-i-1));

  v->count--;
  //Reallocate the memory used
  v->cell = realloc(v->cell, sizeof(lval*) * v->count);
  return x;
}

//taking an element from the list and deleting the rest
lval* lval_take(lval* v, int i) {
  lval* x = lval_pop(v, i);
  lval_del(v);
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
    case LVAL_QEXPR: lval_expr_print(v, '{', '}'); break;
  }
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }

lval* builtin_op(lval* a, char* op) {

  /* 确保所有输入是数字 */
  for (int i = 0; i< a->count; i++) {
    if (a->cell[i]->type != LVAL_NUM) {
      lval_del(a);
      return lval_err("Cannot operate on non-numbers!");
    }
  }
  //pop the first element
  lval* x = lval_pop(a, 0);
  if(strcmp(op, "-") == 0 && a->count == 0) {
    x->num = -x->num;
  }

  /* 仍有元素剩余*/
  while (a->count > 0) {
    //pop the next element
    lval* y = lval_pop(a, 0);
    if (strcmp(op, "+") == 0) { x->num += y->num; }
    if (strcmp(op, "-") == 0) { x->num -= y->num; }
    if (strcmp(op, "*") == 0) { x->num *= y->num; }
    if (strcmp(op, "/") == 0) {
      if(y->num == 0) {
        lval_del(x); lval_del(y);
        x = lval_err("Division by zero!"); break;
      }
      x->num /= y->num;

      lval_del(y);
    }

  }

  lval_del(a);
  return x;
}

lval* lval_eval(lval* v);

lval* lval_eval_sexpr(lval* v) {
  /*求值子结点*/
  for (int i = 0; i< v->count; i++) {
    v->cell[i] = lval_eval(v->cell[i]);
  }

  /*错误检查*/
  for (int i = 0; i < v->count; i++) {
    if (v->cell[i]->type == LVAL_ERR) { return lval_take(v, i); }
  }

  /**/
  if (v->count == 0) { return v; } //空表达式
  if (v->count == 1) { return lval_take(v, 0); }  //一个表达式

  /* 确保第一个表达式是一个Symbol */
  lval* f = lval_pop(v, 0);
  if (f->type != LVAL_SYM) {
    lval_del(f); lval_del(v);
    return lval_err("S Expression Does not start with symbol!");
  }

  /* call builtin_op with operator */
  lval* result = builtin_op(v, f->sym);
  lval_del(f);
  return result;

}

//taking a element from list and popping it out
//leaving what remains
lval* lval_eval(lval* v) {
  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(v); }

  /*其他类型的直接返回*/
  return v;
}



int main(int argc, char const *argv[]) {
  /* polish notation */
  // create some parser
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Symbol = mpc_new("symbol");
  mpc_parser_t* Sexpr = mpc_new("sexpr");
  mpc_parser_t* Qexpr = mpc_new("qexpr");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");

  /* define them with the following Language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                         \
    number   :  /-?[0-9]+/ ;                                  \
    symbol   :  \"list\"| \"head\" | \"tail\"                 \
             |  \"join\" | \"eval\" | '+' | '-' | '*' | '/' ; \
    sexpr    :  '(' <expr>* ')' ;                             \
    qexpr    :  '{' <expr>* '}' ;                             \
    expr     :  <number> | <symbol> | <sexpr> | <qexpr>;      \
    lispy    :  /^/ <expr>* /$/ ;                             \
    ",
    Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

  puts("Lispy Version 0.0.0.0.1");
  puts("Please press Ctrl + C to Exit\n");

  while (1) {
    /* output our prompt and get the input*/
    char* input = readline("lispy--> ");
    /* Add input to history*/
    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)){

      lval* v = lval_eval(lval_read(r.output));
      lval_println(v);
      lval_del(v);
      mpc_ast_delete(r.output);
    }
    else{
      /* otherwise print error*/
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }

    free(input);
  }

  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr,Expr, Lispy);  //undefine and delete parsers
  return 0;
}
/*
→ ./ch9_s_expression
Lispy Version 0.0.0.0.1
Please press Ctrl + C to Exit

lispy--> ()
()
lispy--> (+ 5)
5
lispy--> (- 5)
-5
lispy--> + 1111 (* 100 100)
11111
lispy--> * 100 1000 10000 1000000
1000000000000000
lispy-->
*/
