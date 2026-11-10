%{
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "commun.h"
#include "hashtable.h"
#include "AST.h"

static int local_nbr = 0;
static ast *ast_funcs = nullptr;
static ast *ast_main = nullptr;

int yylex(void);
int yyparse(void);

static void yyerror(const char *s) {
  fprintf(stderr, ANSI_RED "[Error] : %s\n" ANSI_RESET, s);
  exit(EXIT_FAILURE);
}

#define ON_ERROR_DO(exp, mssg) { if (exp) { yyerror(mssg); } }

static size_t str_hashfun(const void *s) {
  size_t h = 0;
  const unsigned char *p = s;
  while (*p != '\0') {
    h = h * 37 + *p++;
  }
  return h;
}

%}

%left OR
%left AND
%nonassoc '<' '>' LEQ GTH EQ NE
%left '+' '-'
%left '*' '/'

%union {
  int integer;
  char *id;
  ast *node;
}

%token <id> STRING IDENTIFIER
%token <integer> INT

%token BEGIN_ALGO END_ALGO
%token SET RETURN CALL
%token IF ELSE OD FI DOWHILE DOFORI
%token PRINTD PRINTS

%type <node> EXPR ADD TERM FACTOR
%type <node> INSTR INSTR_LIST SET_STMT RETURN_STMT
%type <node> IF_STMT WHILE_STMT FOR_STMT PRINTD_STMT PRINTS_STMT
%type <node> ALGO_BLOCK ALGO_LIST PARAM_LIST PARAMS ARG_LIST
%type <node> S FINAL_CALL OPTIONAL_ELSE

%%

S :
      ALGO_LIST FINAL_CALL
      {
        ast_funcs = $1;
        ast_main = $2;
      }
;

ALGO_LIST :
      /* empty */          { $$ = nullptr; }
    | ALGO_LIST ALGO_BLOCK { $$ = ast_link_nodes($1, $2); }
;

ALGO_BLOCK :
      BEGIN_ALGO '{' IDENTIFIER '}'
      {
        if (ht != nullptr) {
          hashtable_dispose(&ht);
        }
        ht = hashtable_empty((int (*)(const void *, const void *)) strcmp,
            (size_t (*)(const void *)) str_hashfun);

        ON_ERROR_DO(ht == nullptr, "[Erreur] : Memory Allocation Failed.\n");
        local_nbr = 0;
        func_info_t *f = func_empty();
        func_set_local_count(f, 0);
        func_set_param_count(f, 0);
        ON_ERROR_DO(hashtable_add(ft, $3, f) == nullptr,
            "[Erreur] : error occured while adding in ft.\n");
        set_current_func_name($3);
      }
      '{' PARAM_LIST '}' INSTR_LIST END_ALGO
      {
        if (!has_guaranteed_return($9)) {
          char error_msg[128];
          snprintf(error_msg, sizeof(error_msg),
              "La fonction '%s' doit obligatoirement avoir un RETURN{}.", $3);
          yyerror(error_msg);
        }
        $$ = ast_create_func($3, $9);
        $$->ctx.local_ht = ht;
        ht = nullptr;
        func_info_t *f = hashtable_search(ft, $3);
        $$->ctx.stack_size = func_get_local_count(f) * 2;
      }
;

PARAM_LIST :
      /* empty */ { $$ = nullptr; }
    | PARAMS      { $$ = $1; }
;

PARAMS :
      IDENTIFIER
      {
        var_t *v = var_empty();
        var_set_offset(v, local_nbr++);
        var_set_initialized(v, 1);
        var_set_type(v, TYPE_INT);
        ON_ERROR_DO(hashtable_add(ht, $1, v) == nullptr,
            "[Erreur] : error occured while adding in ht.\n");
        func_info_t *f = hashtable_search(ft, get_current_func_name());
        func_set_param_count(f, func_get_param_count(f) + 1);
        $$ = ast_create_id($1);
        $$->ctx.offset = var_get_offset(v);
      }
    | IDENTIFIER ','
      {
        var_t *v = var_empty();
        var_set_offset(v, local_nbr++);
        var_set_initialized(v, 1);
        var_set_type(v, TYPE_INT);
        ON_ERROR_DO(hashtable_add(ht, $1, v) == nullptr,
            "[Erreur] : error occured while adding in ht.\n")
        func_info_t *f = hashtable_search(ft, get_current_func_name());
        func_set_param_count(f, func_get_param_count(f) + 1);
      }
      PARAMS
      {
        ast *node = ast_create_id($1);
        var_t *v = hashtable_search(ht, $1);
        node->ctx.offset = var_get_offset(v);
        node->res_type = var_get_type(v);
        $$ = ast_link_nodes(node, $4);
      }
;

INSTR_LIST :
      /* empty */      { $$ = nullptr; }
    | INSTR_LIST INSTR { $$ = ast_link_nodes($1, $2); }
;

INSTR :
      SET_STMT    { $$ = $1; }
    | RETURN_STMT { $$ = $1; }
    | FOR_STMT    { $$ = $1; }
    | PRINTD_STMT { $$ = $1; }
    | PRINTS_STMT { $$ = $1; }
    | WHILE_STMT  { $$ = $1; }
    | IF_STMT     { $$ = $1; }
;

SET_STMT :
      SET '{' IDENTIFIER '}' '{' EXPR '}'
      {
        ON_ERROR_DO($6 == nullptr,
            "Expression invalide dans une affectation.");

        var_t *v = hashtable_search(ht, $3);

        if (v == nullptr) {
          v = var_empty();

          ON_ERROR_DO(v == nullptr,
              "[Erreur] : Memory Allocation Failed.\n");

          var_set_initialized(v, 1);
          var_set_offset(v, local_nbr++);
          var_set_assign_count(v, 0);
          var_set_constant(v, false);

          var_set_type(v, $6->res_type);

          ON_ERROR_DO(
              hashtable_add(ht, $3, v) == nullptr,
              "[Erreur] : error occured while adding in ht.\n"
          );

          func_info_t *f =
              hashtable_search(ft, get_current_func_name());

          if (f != nullptr) {
            func_set_local_count(f, func_get_local_count(f) + 1);
          }
        }

        if (var_get_type(v) != $6->res_type) {
          char msg[256];

          snprintf(msg, sizeof(msg),
              "Type error in assignment of '%s' : expected %s but got %s",
              $3,
              (var_get_type(v) == TYPE_INT ? "INT" : "BOOL"),
              ($6->res_type == TYPE_INT ? "INT" : "BOOL")
          );

          yyerror(msg);
        }

        var_set_initialized(v, 1);
        var_inc_assign_count(v);

        if (var_get_assign_count(v) == 1
            && $6->type == NODE_VALUE) {
          var_set_last_value(v, $6->data.val);
          var_set_constant(v, true);
        } else {
          var_set_constant(v, false);
        }

        $$ = ast_create_assign($3, $6);
        $$->ctx.offset = var_get_offset(v);
      }
;

RETURN_STMT :
      RETURN '{' EXPR '}'
      {
        func_info_t *f_curr = hashtable_search(ft, get_current_func_name());
        if (f_curr != nullptr) {
          func_set_return_type(f_curr, $3->res_type);
        }
        $$ = ast_create_return($3);
      }
;

FOR_STMT :
      DOFORI '{' IDENTIFIER '}'
      {
        var_t *v = hashtable_search(ht, $3);
        if (v == nullptr) {
          v = var_empty();
          var_set_offset(v, local_nbr++);
          var_set_type(v, TYPE_INT);
          var_set_initialized(v, 1);
          var_set_assign_count(v, 1);
          ON_ERROR_DO(hashtable_add(ht, $3, v) == nullptr,
              "[Erreur] : error adding in ht\n");
          func_info_t *curr_f = hashtable_search(ft, get_current_func_name());
          if (curr_f) {
            func_set_local_count(curr_f, func_get_local_count(curr_f) + 1);
          }
        } else {
          var_set_initialized(v, 1);
        }
      }
      '{' EXPR '}' '{' EXPR '}' INSTR_LIST OD
      {
        var_t *v = hashtable_search(ht, $3);
        $$ = ast_create_for($3, $7, $10, $12);
        if ($$ != nullptr && v != nullptr) {
          $$->ctx.offset = var_get_offset(v);
        }
      }
;

WHILE_STMT :
      DOWHILE '{' EXPR '}' INSTR_LIST OD
      {
        $$ = ast_create_while($3, $5);
      }
;

IF_STMT :
      IF '{' EXPR '}' INSTR_LIST OPTIONAL_ELSE FI
      {
        $$ = ast_create_if($3, $5, $6);
      }
;

OPTIONAL_ELSE :
      ELSE INSTR_LIST { $$ = $2; }
    | /* empty */     { $$ = nullptr; }
;

PRINTD_STMT :
      PRINTD '{' EXPR '}'
      {
        $$ = ast_create_call("printd", $3);
      }
;

PRINTS_STMT :
      PRINTS '{' STRING '}'
      {
        $$ = ast_create_call("prints", ast_create_string($3));
      }
;

EXPR :
      EXPR '<' ADD { $$ = ast_create_comp('<', $1, $3); }
    | EXPR '>' ADD { $$ = ast_create_comp('>', $1, $3); }
    | EXPR LEQ ADD { $$ = ast_create_comp('L', $1, $3); }
    | EXPR GTH ADD { $$ = ast_create_comp('G', $1, $3); }
    | EXPR EQ ADD  { $$ = ast_create_comp('=', $1, $3); }
    | EXPR NE ADD  { $$ = ast_create_comp('!', $1, $3); }
    | ADD          { $$ = $1; }
;

ADD :
      ADD '+' TERM { $$ = ast_create_op('+', $1, $3); }
    | ADD '-' TERM { $$ = ast_create_op('-', $1, $3); }
    | TERM         { $$ = $1; }
;

TERM :
      TERM '*' FACTOR { $$ = ast_create_op('*', $1, $3); }
    | TERM '/' FACTOR { $$ = ast_create_op('/', $1, $3); }
    | FACTOR          { $$ = $1; }
;

FACTOR :
      INT
      {
        $$ = ast_create_value($1);
      }

    | CALL '{' IDENTIFIER '}' '{' ARG_LIST '}'
      {
        func_info_t *f = hashtable_search(ft, $3);

        if (f == nullptr) {
          char msg[128];

          snprintf(msg, sizeof(msg),
              "Appel a une fonction inconnue : '%s'", $3);

          yyerror(msg);
        }

        int n = 0;

        for (ast *a = $6; a != nullptr; a = a->next) {
          n++;
        }

        int p = func_get_param_count(f);

        if (n != p) {
          char msg[256];

          snprintf(msg, sizeof(msg),
              "La fonction '%s' attend %d argument(s), "
              "mais %d fourni(s).",
              $3, p, n);

          yyerror(msg);
        }

        int i = 1;

        for (ast *a = $6; a != nullptr; a = a->next, i++) {
          if (a->res_type != TYPE_INT) {
            char msg[256];

            snprintf(msg, sizeof(msg),
                "Type error in call to '%s' : argument %d "
                "expected INT but got %s",
                $3, i,
                (a->res_type == TYPE_BOOL ? "BOOL" : "UNKNOWN"));

            yyerror(msg);
          }
        }
        $$ = ast_create_call($3, $6);
        $$->res_type = func_get_return_type(f);
      }

    | '(' EXPR ')'
      {
        $$ = $2;
      }

    | IDENTIFIER
      {
        var_t *v = hashtable_search(ht, $1);

        if (v == nullptr) {
          char msg[128];

          snprintf(msg, sizeof(msg),
              "Variable '%s' non declaree !", $1);

          yyerror(msg);
        }

        $$ = ast_create_id($1);
        $$->ctx.offset = var_get_offset(v);
        $$->res_type = var_get_type(v);
      }
;

FINAL_CALL :
      CALL '{' IDENTIFIER '}' '{' ARG_LIST '}'
      {
        func_info_t *f = hashtable_search(ft, $3);

        if (f == nullptr) {
          char msg[128];

          snprintf(msg, sizeof(msg),
              "Appel a une fonction inconnue : '%s'", $3);

          yyerror(msg);
        }

        int n = 0;

        for (ast *a = $6; a != nullptr; a = a->next) {
          n++;
        }

        int p = func_get_param_count(f);

        if (n != p) {
          char msg[256];

          snprintf(msg, sizeof(msg),
              "La fonction '%s' attend %d argument(s), "
              "mais %d fourni(s).",
              $3, p, n);

          yyerror(msg);
        }

        int i = 1;

        for (ast *a = $6; a != nullptr; a = a->next, i++) {
          if (a->res_type != TYPE_INT) {
            char msg[256];

            snprintf(msg, sizeof(msg),
                "Type error in call to '%s' : argument %d "
                "expected INT but got %s",
                $3, i,
                (a->res_type == TYPE_BOOL ? "BOOL" : "UNKNOWN"));

            yyerror(msg);
          }
        }

        ast *c = ast_create_call($3, $6);
        c->res_type = func_get_return_type(f);

        $$ = ast_create_call("printd", c);
      }
;

ARG_LIST :
      /* empty */       { $$ = nullptr; }
    | EXPR              { $$ = $1; }
    | EXPR ',' ARG_LIST { $$ = ast_link_nodes($1, $3); }
;

%%

int main(void) {
  ft = hashtable_empty(
      (int (*)(const void *, const void *)) strcmp,
      (size_t (*)(const void *)) str_hashfun
  );
  ON_ERROR_DO(ft == nullptr, "[Erreur] : Memory Allocation Failed.\n")
  if (yyparse() == 0 && ast_funcs != nullptr) {

#ifdef OPTIMIZE
    bool global_change;
    do {
      tree_changed = false;
      ast_funcs = ast_reduce(ast_funcs, nullptr);
      global_change = tree_changed;
    } while (global_change);
    ast_funcs = ast_clean(ast_funcs);
    ast *curr = ast_funcs;
    while (curr) {
      if (curr->type == NODE_FUNC) {
        ast_recompute_offsets(curr);
      }
      curr = curr->next;
    }
#endif

    ast_dump_tree(ast_funcs, 0);
    ast_dump_tree(ast_main, 0);
    printf(
        "\tconst ax,main_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-__-_-_-_-_-_-_-_-_-_-_-_-_\n");
    printf("\tjmp ax\n");
    ast_generate_asm(ast_funcs);
    printf(":main_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-__-_-_-_-_-_-_-_-_-_-_-_-_\n");
    printf("\tconst bp,stack\n");
    printf("\tconst sp,stack\n");
    printf("\tconst ax,2\n");
    printf("\tsub sp,ax\n");
    if (ast_main) {
      ast_generate_asm(ast_main);
      printf("\tconst ax,str_nl\n");
      printf("\tcallprintfs ax\n");
    }
    printf("\tend\n");
    printf(":str_true\n@string \"True\"\n");
    printf(":str_false\n@string \"False\"\n");
    printf(":str_nl\n@string \"\\n\"\n");
    printf(":stack\n");
    printf("@int 0\n");
    if (ht != nullptr) {
      hashtable_dispose(&ht);
    }
    if (ft != nullptr) {
      hashtable_dispose(&ft);
    }
  } else {
    fprintf(stderr, "***Warning! Parsing failed.\n");
    return EXIT_FAILURE;
  }
  return 0;
}
