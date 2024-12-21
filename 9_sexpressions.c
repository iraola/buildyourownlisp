#include <stdio.h>
#include <stdlib.h>
#include "mpc.h"

/* If we are on Windows, compile these functions */
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* Fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer) + 1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy) - 1] = '/0';
  return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else
#include <editline/readline.h>
#include <editline/history.h>
#endif

/* ---------- START OF CODE ---------- */


/* Create enumeration of possible lval types */
enum { LVAL_NUM, LVAL_ERR };

/* Create enumeration of possible error types */
enum { LERR_ZERO_DIV, LERR_BAD_OP, LERR_BAD_NUM };

/* Create new Lisp value struct */
typedef struct {
  int type;
  long num;
  int err;
} lval;

/* Create new number type lval */
lval lval_num(long x) {
  lval v;
  v.type = LVAL_NUM;
  v.num = x;
  return v;
}

/* Create new error type lval */
lval lval_err(int x) {
  lval v;
  v.type = LVAL_ERR;
  v.err = x;
  return v;
}

/* Print lval */
void lval_print(lval v) {
  switch (v.type) {
    case LVAL_NUM:
      printf("%li", v.num);
      break;
    case LVAL_ERR:
      if (v.err == LERR_ZERO_DIV) {
        printf("Error: division by zero!");
      }
      if (v.err == LERR_BAD_OP) {
        printf("Error: invalid operator!");
      }
      if (v.err == LERR_BAD_NUM ){
        printf("Error: invalid number!");
      }
  }
}

/* Print lval followed by newline */
void lval_println(lval v) { lval_print(v);  putchar('\n'); }

/* Return the total number of nodes in a tree */
int number_of_nodes(mpc_ast_t* t) {
  if (t->children_num == 0){ return 1; }
  else {
    int total = 1;
    for (int i = 0; i < t->children_num; i++) {
      total = total + number_of_nodes(t->children[i]);
    }
    return total;
  }
}

/* Return the number of leaves in a tree */
int number_of_leaves(mpc_ast_t* t) {
  if (t->children_num == 0) { return 1; }
  else {
    int total = 0;
    for (int i = 0; i < t->children_num; i++) {
      total = total + number_of_leaves(t->children[i]);
    }
    return total;
  }
}

lval eval_op(lval x, char* op, lval y) {
  /* If either value is an error return it */
  if (x.type == LVAL_ERR) { return x; }
  if (y.type == LVAL_ERR) { return y; }

  /* Otherwise do the required operation */
  if (strcmp(op, "+") == 0) { return lval_num(x.num + y.num); }
  if (strcmp(op, "-") == 0) { return lval_num(x.num - y.num); }
  if (strcmp(op, "*") == 0) { return lval_num(x.num * y.num); }
  if (strcmp(op, "/") == 0) {
    /* If second operant is zero return error */
    return y.num == 0 
      ? lval_err(LERR_ZERO_DIV) 
      : lval_num(x.num / y.num); 
  }

  /* Special operators */
  if (strcmp(op, "%") == 0) { return lval_num(x.num % y.num); }
  if (strcmp(op, "min") == 0) {
    if (x.num > y.num) { return lval_num(y.num); }
    else { return lval_num(x.num); }
  }
  if (strcmp(op, "max") == 0) {
    if (x.num < y.num) { return lval_num(y.num); }
    else { return lval_num(x.num); }
  }

  /* If reaching this point operator was not recognized */
  return lval_err(LERR_BAD_OP);
}

lval eval(mpc_ast_t* t) {
  /* If tagged as number return it directly. */
  if (strstr(t->tag, "number")) {
    errno = 0;  // Global variable modified by strtol
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? lval_num(x) : lval_err(LERR_BAD_NUM);
  }

  /* The operator is always second child. */
  char* op = t->children[1]->contents;

  /* We store the third child in `x` */
  lval x = eval(t->children[2]);

  /* Iterate the remaining children and combining. */
  int i = 3;
  while (strstr(t->children[i]->tag, "expr")) {
    x = eval_op(x, op, eval(t->children[i]));
    i++;
  }

  return x;
}

int main(int argc, char** argv) {
  /* Create some parsers */
  mpc_parser_t* Number = mpc_new("number");
  mpc_parser_t* Operator = mpc_new("operator");
  mpc_parser_t* Expr = mpc_new("expr");
  mpc_parser_t* Lispy = mpc_new("lispy");
  
  /* Now define the previous parser with the following language */
  mpca_lang(MPCA_LANG_DEFAULT,
    "								\
      number	: /-?[0-9]+(\\.[0-9]+)?/ ;			\
      operator  : '+' | '-' | '*' | '/' | '%' |			\
      		  \"add\" | \"sub\" | \"mul\" | \"div\" |	\
            \"min\" | \"max\" ;                      \
      expr	: <number> | '(' <operator> <expr>+ ')'	;	\
      lispy	: /^/ <operator> <expr>+ /$/ ;			\
    ",
    Number, Operator, Expr, Lispy);
 
  /* Print Version and Exit Information */
  puts("Lispy Version 0.0.0.0.1");
  puts("Press Ctrl+C to Exit\n");
  
  while (1) {
    /* Output our prompt and get input. readline allows users using left/right arrows */
    char* input = readline("lispy> ");
    
    /* Add input to history so that user can use the up/down arrows to move through older commands */
    add_history(input);
    
    /* Echo input back to user */
    mpc_result_t r;
    if (mpc_parse("<stdin>", input, Lispy, &r)) {
      /* Print AST specific parts */
      mpc_ast_t* a = r.output;
      printf("\n(recursion) NUMBER OF NODES: %i\n", number_of_nodes(a)); 
      printf("(recursion) NUMBER OF LEAVES: %i\n", number_of_leaves(a));
      /* On success print the AST */
      mpc_ast_print(a);
      lval result = eval(a);
      lval_println(result);
      mpc_ast_delete(a);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
    }
    
    /* Free retrieved input */
    free(input);
  }
  
  /* Undefine and delete our parsers */
  mpc_cleanup(4, Number, Operator, Expr, Lispy);
  
  return 0;
}
