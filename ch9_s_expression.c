#include<stdio.h>
#include<stdlib.h>
#include"mpc.h"

#include<editline/readline.h>

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;

/* 创建lval的可能的值的枚举类型 */
enum { LVAL_NUM, LVAL_ERR, LVAL_SYM,
       LVAL_FUN, LVAL_SEXPR, LVAL_QEXPR };


/*
  声明了一个类型为lval*, 名为lbuiltind的函数指针
*/
typedef lval*(*lbuiltin)(lenv*, lval*);


//lval Lisp Value
struct lval {
  int type;  //Number or Error
  long num;  //value
  char* err; //error string
  char* sym;

  lbuiltin fun;
  /* Count and Pointer to a list of "lval*" */
  int count;
  struct lval** cell;
};


/* env structure */
struct lenv {
  int count;
  char* syms;
  lval** vals;
};


lenv* lenv_new(void) {
  lenv* e = malloc(sizeof(lenv));
  e->count = 0;
  e->syms = NULL;
  e->vals = NULL;
  return e;
}


#define LASSERT(args, cond, err) \
  if (!(cond)) { lval_del(args); return lval_err(err); }


/* constructor of LVAL_FUN */
lval* lval_fun(lbuiltin func) {
  lval* v = malloc(sizeof(lval));
  v->type = LVAL_FUN;
  v->fun = func;
  return v;
}


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
    case LVAL_FUN: break;
  }
  /* 释放lval结构体本身被分配的内存*/
  free(v);
}


void lenv_del(lenv* e) {
  for(int i = 0; i < e->count; i++) {
    free(e->syms[i]);
    lval_del(e->vals[i]);
  }
  free(e->syms);
  free(e->vals);
  free(e);
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


/* 从环境中拿东西或者往环境中添加东西 */
lval* lval_copy(lval* v) {

  lval* x = malloc(sizeof(lval));
  x->type = v->type;

  switch(v->type) {
    /* 函数和数字直接拷贝 */
    case LVAL_FUN: x->fun = v->fun; break;
    case LVAL_NUM: x->num = v->num; break;

    /* 拷贝字符串用malloc 和 strcpy */
    case LVAL_ERR:
      x->err = malloc(strlen(v->err) + 1);
      strcpy(x->err, v->err); break;

    case LVAL_SYM:
      x->sym = malloc(strlen(v->sym) + 1);
      strcpy(x->sym, v->sym); break;

    /* 拷贝列表通过拷贝每一个子表达式 */
    case LVAL_SEXPR:
    case LVAL_QEXPR:
      x->count = v->count;
      x->cell = malloc(sizeof(lval*) * x->count);
      for (int i=0; i<x->count; i++) {
        x->cell[i] = lval_copy(v->cell[i]);
      }
      break;

  }

  return x;
}

/* get value from the env */
lval* lenv_get(lenv* e, lval* k) {

  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0) {
      return lval_copy(e->vals[i]);
    }
  }

  /* 如果没找到任何符号返回错误 */
  return lval_err("unbound symbol! ");
}


void lenv_put(lenv* e, lval* k, lval* v) {

/* to see if variable already exists */
  for (int i = 0; i < e->count; i++) {
    if (strcmp(e->syms[i], k->sym) == 0) {
      lval_del(e->vals[i]);
      e->vals[i] = lval_copy(v);
      return;
    }
  }

  /* 如果没找到已存在的entry， 为新的entry分配内存 */
  e->count ++;
  e->vals = realloc(e->vals, sizeof(lval*) * e->count);
  e->syms = realloc(e->syms, sizeof(char*) * e->count);

  /* 将k， v的内容拷贝到新分配的位置 */
  e->syms[e->count-1] = malloc(strlen(k->sym)+1);
  strcpy(e->syms[e->count-1], k->sym);
  e->vals[e->count-1] = lval_copy(v);

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
    case LVAL_FUN: printf("<function>"); break;
  }
}

void lval_println(lval* v) { lval_print(v); putchar('\n'); }

/*repeatedly pop and delete the item at index 1 util there nothing left in list*/
lval* builtin_head(lenv* e, lval* a) {
  LASSERT(a, a->count == 1,
    "Function 'head' passed too many arguments! ");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'head' passed incorrect type! ");
  LASSERT(a, a->cell[0]->count != 0,
    "Function 'head' paased {}! ");

  lval* v = lval_take(a, 0);
  while (v->count > 1) { lval_del(lval_pop(v, 1));}
  return v;
}

/*
pop and delete item at index 0, leaving the tail remaining
*/
lval* builtin_tail(lenv* e, lval* a) {
  LASSERT(a, a->count == 1,
    "Function 'tail' passed too many arguments! ");
  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'tail' passed incorrect type! ");
  LASSERT(a, a->cell[0]->count != 0,
    "Function 'tail' paased {}! ");

  lval* v = lval_take(a, 0);
  lval_del(lval_pop(v, 0));
  return v;
}

lval* builtin_list(lenv* e, lval* a) {
  a->type = LVAL_QEXPR;
  return a;
}

lval* lval_eval(lenv* e, lval* v);

lval* builtin_eval(lenv* e, lval* a) {
  LASSERT(a, a->count == 1,
    "Function 'eval' passed too many arguments! ");

  LASSERT(a, a->cell[0]->type == LVAL_QEXPR,
    "Function 'eval' passed the incorrect type! ");

  lval* x = lval_take(a, 0);
  x->type = LVAL_SEXPR;
  return lval_eval(e, x);
}


lval* lval_join(lval* x, lval* y) {

  /* 把每一个y中的cell添加到x中 */
  while(y->count) {
    x = lval_add(x, lval_pop(y, 0));
  }

  /* 删除空的y，返回x */
  lval_del(y);
  return x;
}


lval* builtin_join(lenv* e, lval* a) {

  for (int i = 0; i < a->count; i++) {
    LASSERT(a, a->cell[i]->type == LVAL_QEXPR,
      "Function 'join' passed incorrect type! ");
  }

  lval* x = lval_pop(a, 0);
  while(a->count) {
    x = lval_join(x, lval_pop(a, 0));
  }
  lval_del(a);
  return x;
}


lval* builtin_op(lenv* e, lval* a, char* op) {

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


lval* builtin_add(lenv* e, lval* a) {
  return builtin_op(e, a, "+");
}

lval* builtin_sub(lenv* e, lval* a) {
  return builtin_op(e, a, "-");
}

lval* builtin_mul(lenv* e, lval* a) {
  return builtin_op(e, a, "*");
}

lval* builtin_div(lenv* e, lval* a) {
  return builtin_op(e, a, "/");
}


void lenv_add_builtin(lenv* e, char* name, lbuiltin func) {
  lval* k = lval_sym(name);
  lval* v = lval_fun(func);
  lenv_put(e, k, v);
  lval_del(k); lval_del(v);
}

void lenv_add_builtins(lenv* e) {
  /**/
  lenv_add_builtin(e, "list", builtin_list);
  lenv_add_builtin(e, "head", builtin_head);
  lenv_add_builtin(e, "tail", builtin_tail);
  lenv_add_builtin(e, "eval", builtin_eval);
  lenv_add_builtin(e, "join", builtin_join);

  lenv_add_builtin(e, "+", builtin_add);
  lenv_add_builtin(e, "-", builtin_sub);
  lenv_add_builtin(e, "*", builtin_mul);
  lenv_add_builtin(e, "/", builtin_div);
}

// lval* builtin(lval* a, char* func) {
//   if (strcmp("list", func) == 0) { return builtin_list(a); }
//   if (strcmp("head", func) == 0) { return builtin_head(a); }
//   if (strcmp("tail", func) == 0) { return builtin_tail(a); }
//   if (strcmp("join", func) == 0) { return builtin_join(a); }
//   if (strcmp("eval", func) == 0) { return builtin_eval(a); }
//   if (strstr("+-*/", func)) { return builtin_op(a, func); }
//   lval_del(a);
//   return lval_err("Unknown Function! ");
// }


lval* lval_eval_sexpr(lenv* e, lval* v) {
  /*求值子结点*/
  for (int i = 0; i< v->count; i++) {
    v->cell[i] = lval_eval(e, v->cell[i]);
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
  if (f->type != LVAL_FUN) {
    lval_del(f); lval_del(v);
    return lval_err("first element is not a function! ");
  }

  /* call builtin with operator */
  lval* result = f->fun(e, v);
  lval_del(f);
  return result;

}

//taking a element from list and popping it out
//leaving what remains
lval* lval_eval(lenv* e, lval* v) {

  if (v->type == LVAL_SYM) {
    lval* x = lenv_get(e, v);
    lval_del(v);
    return x;
  }

  if (v->type == LVAL_SEXPR) { return lval_eval_sexpr(e, v); }

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

  /* define them with the following Language
  [a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+
  */
  mpca_lang(MPCA_LANG_DEFAULT,
    "                                                         \
    number   :  /-?[0-9]+/ ;                                  \
    symbol   :  /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ; \
    sexpr    :  '(' <expr>* ')' ;                             \
    qexpr    :  '{' <expr>* '}' ;                             \
    expr     :  <number> | <symbol> | <sexpr> | <qexpr>;      \
    lispy    :  /^/ <expr>* /$/ ;                             \
    ",
    Number, Symbol, Sexpr, Qexpr, Expr, Lispy);

  puts("Lispy Version 0.0.0.0.1");
  puts("Please press Ctrl + C to Exit\n");

  lenv* e = lenv_new();
  lenv_add_builtins(e);
  while (1) {
    /* output our prompt and get the input*/
    char* input = readline("lispy--> ");
    /* Add input to history*/
    add_history(input);

    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)){

      lval* v = lval_eval(e, lval_read(r.output));
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

  lenv_del(e);
  mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr,Expr, Lispy);  //undefine and delete parsers
  return 0;
}
/*
lispy--> list 1 2 3 4
{1 2 3 4}
lispy--> {head {list 1 2 3 4}}
{head {list 1 2 3 4}}
lispy--> eval {head (list 1 2 3 4)}
{1}
lispy--> tail {tail tail tail}
{tail tail}
lispy--> eval (tail {tail tail {5 6 7}})
{6 7}
lispy--> eval (head {(+ 1 2) (* 10 10)})
3
lispy--> eval (tail {(+ 1 2) (* 10 10)})
100
*/
