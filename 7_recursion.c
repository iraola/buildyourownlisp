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

long eval_op(long x, char* op, long y) {
  if (strcmp(op, "+") == 0) { return x + y; }
  if (strcmp(op, "-") == 0) { return x - y; }
  if (strcmp(op, "*") == 0) { return x * y; }
  if (strcmp(op, "/") == 0) { return x / y; }
  if (strcmp(op, "%") == 0) { return x % y; }
  if (strcmp(op, "min") == 0) {
    if (x > y) { return y; }
    else { return x; }
  }
  if (strcmp(op, "max") == 0) {
    if (x < y) { return y; }
    else { return x; }
  }
  return 0;
}

long eval(mpc_ast_t* t) {
  /* If tagged as number return it directly. */
  if (strstr(t->tag, "number")) {
    return atoi(t->contents);
  }

  /* The operator is always second child. */
  char* op = t->children[1]->contents;

  /* We store the third child in `x` */
  long x = eval(t->children[2]);

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
      /*
      // printf("Abstract Syntax Tree tag is %s\n", a->tag);
      // printf("AST contents are %s\n", a->contents);
      // printf("Number of children is %i\n", a->children_num);
      /* Get first children */
      // mpc_ast_t* c0 = a->children[0];
      // printf("\nFirst children is %s\n", c0->tag);
      // printf("First children contents is %s\n", c0->contents);
      // printf("First children num children is %i\n", c0->children_num);
      /* Use recursion */
      printf("\n(recursion) NUMBER OF NODES: %i\n", number_of_nodes(a)); 
      printf("(recursion) NUMBER OF LEAVES: %i\n", number_of_leaves(a));
      
      /* On success print the AST */
      mpc_ast_print(a);
      long result = eval(a);
      printf("%li\n", result);
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
