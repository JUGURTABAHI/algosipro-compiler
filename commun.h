#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include "hashtable.h"

#define ANSI_RESET       "\033[0m"
#define ANSI_BOLD        "\033[1m"
#define ANSI_RED         "\033[1;4;31m"
#define TREE_BRANCH      "\033[38;2;80;80;80m"
#define TYPE_COLOR       "\033[1;38;2;57;255;20m"
#define ID_COLOR         "\033[1;38;2;0;255;255m"
#define VAL_COLOR        "\033[1;38;2;255;0;255m"
#define STR_COLOR        "\033[38;2;255;215;0m"
#define OP_COLOR         "\033[1;38;2;245;20;147m"
#define ANSI_PARAM       "\033[38;2;255;165;0m"
#define ANSI_LOCAL       "\033[38;2;248;248;242m"

typedef enum {
  TYPE_INT,
  TYPE_BOOL,
} type_s;

typedef struct var_t var_t;
typedef struct func_info_t func_info_t;

extern hashtable *ft;
extern hashtable *ht;

var_t *var_empty();
void var_dispose(var_t *v);

int var_get_offset(var_t *v);
void var_set_offset(var_t *v, int off);

int var_get_type(var_t *v);
void var_set_type(var_t *v, int type);

int var_get_initialized(var_t *v);
void var_set_initialized(var_t *v, int init);

int var_get_assign_count(var_t *v);
void var_set_assign_count(var_t *v, int count);
void var_inc_assign_count(var_t *v);

int var_get_last_value(var_t *v);
void var_set_last_value(var_t *v, int val);

bool var_get_constant(var_t *v);
void var_set_constant(var_t *v, bool constant);

func_info_t *func_empty();
void func_dispose(func_info_t *f);

int func_get_return_type(func_info_t *f);
void func_set_return_type(func_info_t *f, int type);

int func_get_local_count(func_info_t *f);
void func_set_local_count(func_info_t *f, int count);

int func_get_param_count(func_info_t *f);
void func_set_param_count(func_info_t *f, int count);

int func_get_used(func_info_t *f);
void func_set_used(func_info_t *f, int used);

void set_current_func_name(const char *name);
const char *get_current_func_name();

void *func_get_body(func_info_t *f);
void func_set_body(func_info_t *f, void *body);

bool var_get_used(var_t *v);
void var_set_used(var_t *v, bool used);

#endif /* COMMUN_H */
