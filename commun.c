#include <stdio.h>
#include <stdlib.h>

#include "commun.h"

static const char *_current_func_name = nullptr;
hashtable *ft = nullptr; // La table des fonctions
hashtable *ht = nullptr; // La table des variables (courante)

struct var_t {
  int offset;
  int type;
  int is_initialized;
  int assign_count;
  int last_value;
  bool is_constant;
  bool is_used;
};

struct func_info_t {
  int return_type;
  int local_count;
  int param_count;
  bool is_used;
  void *func_body;
};

var_t *var_empty() {
  var_t *v = calloc(1, sizeof(*v));
  if (v == nullptr) {
    fprintf(stderr,
        ANSI_RED "[Erreur] : Memory Allocation Failed for a variable.\n"
        ANSI_RESET);
    exit(EXIT_FAILURE);
  }
  return v;
}

void var_dispose(var_t *v) {
  if (v != nullptr) {
    free(v);
  }
}

int var_get_offset(var_t *v) {
  return (v != nullptr) ? v->offset : 0;
}

void var_set_offset(var_t *v, int off) {
  if (v != nullptr) {
    v->offset = off;
  }
}

int var_get_type(var_t *v) {
  return (v != nullptr) ? v->type : 0;
}

void var_set_type(var_t *v, int type) {
  if (v != nullptr) {
    v->type = type;
  }
}

int var_get_initialized(var_t *v) {
  return (v != nullptr) ? v->is_initialized : 0;
}

void var_set_initialized(var_t *v, int init) {
  if (v != nullptr) {
    v->is_initialized = init;
  }
}

int var_get_assign_count(var_t *v) {
  return (v != nullptr) ? v->assign_count : 0;
}

void var_set_assign_count(var_t *v, int count) {
  if (v != nullptr) {
    v->assign_count = count;
  }
}

void var_inc_assign_count(var_t *v) {
  if (v != nullptr) {
    v->assign_count++;
  }
}

int var_get_last_value(var_t *v) {
  return (v != nullptr) ? v->last_value : 0;
}

void var_set_last_value(var_t *v, int val) {
  if (v != nullptr) {
    v->last_value = val;
  }
}

bool var_get_constant(var_t *v) {
  return (v != nullptr) ? v->is_constant : false;
}

void var_set_constant(var_t *v, bool constant) {
  if (v != nullptr) {
    v->is_constant = constant;
  }
}

func_info_t *func_empty() {
  func_info_t *f = calloc(1, sizeof(*f));
  if (f == nullptr) {
    fprintf(stderr, ANSI_RED
        "[Erreur] : Memory Allocation Failed for a function.\n" ANSI_RESET);
    exit(EXIT_FAILURE);
  }
  return f;
}

void func_dispose(func_info_t *f) {
  if (f != nullptr) {
    free(f);
  }
}

int func_get_return_type(func_info_t *f) {
  return (f != nullptr) ? f->return_type : 0;
}

void func_set_return_type(func_info_t *f, int type) {
  if (f != nullptr) {
    f->return_type = type;
  }
}

int func_get_local_count(func_info_t *f) {
  return (f != nullptr) ? f->local_count : 0;
}

void func_set_local_count(func_info_t *f, int count) {
  if (f != nullptr) {
    f->local_count = count;
  }
}

int func_get_param_count(func_info_t *f) {
  return (f != nullptr) ? f->param_count : 0;
}

void func_set_param_count(func_info_t *f, int count) {
  if (f != nullptr) {
    f->param_count = count;
  }
}

int func_get_used(func_info_t *f) {
  return (f != nullptr) ? f->is_used : 0;
}

void func_set_used(func_info_t *f, int used) {
  if (f != nullptr) {
    f->is_used = used;
  }
}

void set_current_func_name(const char *name) {
  _current_func_name = name;
}

const char *get_current_func_name() {
  return _current_func_name;
}

void *func_get_body(func_info_t *f) {
  if (f == nullptr) {
    return nullptr;
  }
  return f->func_body;
}

void func_set_body(func_info_t *f, void *body) {
  if (f != nullptr) {
    f->func_body = body;
  }
}

bool var_get_used(var_t *v) {
  return (v != nullptr) ? v->is_used : false;
}

void var_set_used(var_t *v, bool used) {
  if (v != nullptr) {
    v->is_used = used;
  }
}
