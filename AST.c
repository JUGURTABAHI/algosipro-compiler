#include "AST.h"
#include "hashtable.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>

static const char *current_func_name = nullptr;
bool tree_changed = false;

static int new_num() {
  static int k = 0;
  return ++k;
}

static void ast_free(ast *n) {
  if (n == nullptr) {
    return;
  }
  ast_free(n->b1);
  ast_free(n->b2);
  ast_free(n->b3);
  ast_free(n->next);
  if ((n->type == NODE_ID || n->type == NODE_STRING)
      && n->data.name != nullptr) {
    free(n->data.name);
  }
  free(n);
}

static ast *ast_allocate(node_t type) {
  ast *n = calloc(1, sizeof(*n));
  if (n == nullptr) {
    fprintf(stderr,
        ANSI_RED "[Erreur], Memory Allocation Failed.\n" ANSI_RESET);
    exit(EXIT_FAILURE);
  }
  n->type = type;
  return n;
}

ast *ast_create_value(int v) {
  ast *n = ast_allocate(NODE_VALUE);
  n->data.val = v;
  n->res_type = TYPE_INT;
  return n;
}

ast *ast_create_id(const char *name) {
  ast *n = ast_allocate(NODE_ID);
  n->data.name = strdup(name);
  return n;
}

ast *ast_create_string(char *str) {
  ast *n = ast_allocate(NODE_STRING);
  n->data.name = strdup(str);
  n->ctx.index = new_num();
  return n;
}

#ifdef OPTIMIZE

//  is_constant : Vérifie si le noeud n existe, et que il s'agit d'une constante
//    numérique.
static bool is_constant(ast *n) {
  return n != nullptr && n->type == NODE_VALUE;
}

//  is_value : Vérifie si la valeur de la constante est égale a v.
static bool is_value(ast *n, int v) {
  return is_constant(n) && n->data.val == v;
}

//  is_same_id : Vérifie si le noeud pointé par l possède le meme id que
//    celui de noued pointé par r
static bool is_same_id(ast *l, ast *r) {
  if (l == r) {
    return l && l->type == NODE_ID;
  }
  return l && r && l->type == NODE_ID && r->type == NODE_ID
    && strcmp(l->data.name, r->data.name) == 0;
}

#endif

ast *ast_create_op(char op, ast *l, ast *r) {
  if (l == nullptr || r == nullptr) {
    return nullptr;
  }
  if (l->res_type != TYPE_INT || r->res_type != TYPE_INT) {
    fprintf(stderr,
        ANSI_RED
        "[Erreur de Type] : l'operateur '%c' attend deux INT "
        "(recu : %s et %s).\n"
        ANSI_RESET,
        op,
        (l->res_type == TYPE_INT ? "INT" : "BOOL"),
        (r->res_type == TYPE_INT ? "INT" : "BOOL")
        );
    exit(EXIT_FAILURE);
  }
#ifdef OPTIMIZE
  // 1. CONSTANT PROPAGATION : On fait le calcul si ce sont deux
  // valeurs numériques.
  if (is_constant(l) && is_constant(r)) {
    int v1 = l->data.val;
    int v2 = r->data.val;
    int res = 0;
    switch (op) {
      case '+':
        res = v1 + v2;
        break;
      case '-':
        res = v1 - v2;
        break;
      case '*':
        res = v1 * v2;
        break;
      case '/':
        if (v2 == 0) {
          fprintf(stderr,
              ANSI_RED
              "[Erreur] : division par zero dans une expression constante.\n"
              ANSI_RESET);
          exit(EXIT_FAILURE);
        }
        if (v1 == INT_MIN && v2 == -1) {
          fprintf(stderr,
              ANSI_RED
              "[Erreur] : debordement dans une division constante.\n"
              ANSI_RESET);
          exit(EXIT_FAILURE);
        }
        res = v1 / v2;
        break;
      default:
        fprintf(stderr,
            ANSI_RED "[Erreur] : operateur inconnu '%c'.\n" ANSI_RESET,
            op);
        exit(EXIT_FAILURE);
    }
    free(l);
    free(r);
    return ast_create_value(res);
  }
  // 2. IDENTITÉ : x - x = 0 et x / x = 1
  if (op == '-' && is_same_id(l, r)) {
    if (l == r) {
      ast_free(l);
    } else {
      ast_free(l);
      ast_free(r);
    }
    return ast_create_value(0);
  }
  // 4. ÉLÉMENTS NEUTRES À DROITE : x + 0, x - 0, x * 1, x / 1
  if ((is_value(r, 0) && (op == '+' || op == '-'))
      || (is_value(r, 1) && (op == '*' || op == '/'))) {
    free(r);
    return l;
  }
  // 5. ÉLÉMENTS NEUTRES À GAUCHE : 0 + x, 1 * x
  if ((is_value(l, 0) && op == '+')
      || (is_value(l, 1) && op == '*')) {
    free(l);
    return r;
  }
  // 6. ASSOCIATIVITÉ (Constant Folding)
  // A. Pour l'addition : (x + C1) + C2 => x + (C1 + C2)
  if (op == '+' && is_constant(r) && l != nullptr && l->type == NODE_OP
      && l->data.op == '+') {
    if (is_constant(l->b2)) {
      int c1 = l->b2->data.val;
      int c2 = r->data.val;
      ast *new_const = ast_create_value(c1 + c2);
      ast *x_node = l->b1;
      free(l->b2);
      free(l);
      free(r);
      return ast_create_op('+', x_node, new_const);
    }
  }
  // B. Pour la multiplication : (x * C1) * C2 => x * (C1 * C2)
  if (op == '*' && is_constant(r) && l != nullptr && l->type == NODE_OP
      && l->data.op == '*') {
    if (is_constant(l->b2)) {
      int c1 = l->b2->data.val;
      int c2 = r->data.val;
      ast *new_const = ast_create_value(c1 * c2);
      ast *x_node = l->b1;
      free(l->b2);
      free(l);
      free(r);
      return ast_create_op('*', x_node, new_const);
    }
  }
  // C. Associativité avec X au milieu : (C1 + x) + C2 => x + (C1 + C2)
  if (op == '+' && is_constant(r) && l != nullptr && l->type == NODE_OP
      && l->data.op == '+') {
    if (is_constant(l->b1)) {
      int c1 = l->b1->data.val;
      int c2 = r->data.val;
      ast *new_const = ast_create_value(c1 + c2);
      ast *x_node = l->b2;
      free(l->b1);
      free(l);
      free(r);
      return ast_create_op('+', x_node, new_const);
    }
  }
  // D. Associativité inversée : C1 + (x + C2) => x + (C1 + C2)
  if (op == '+' && is_constant(l) && r != nullptr && r->type == NODE_OP
      && r->data.op == '+') {
    if (is_constant(r->b2)) {
      int c1 = l->data.val;
      int c2 = r->b2->data.val;
      ast *new_const = ast_create_value(c1 + c2);
      ast *x_node = r->b1;
      free(r->b2);
      free(r);
      free(l);
      return ast_create_op('+', x_node, new_const);
    }
  }
#endif
  ast *n = ast_allocate(NODE_OP);
  n->data.op = op;
  n->res_type = TYPE_INT;
  n->b1 = l;
  n->b2 = r;
  return n;
}

ast *ast_create_comp(char op, ast *l, ast *r) {
#ifdef OPTIMIZE
  if (l && l->type == NODE_VALUE && r && r->type == NODE_VALUE) {
    int val_l = l->data.val;
    int val_r = r->data.val;
    int res = 0;
    switch (op) {
      case '>':
        res = (val_l > val_r);
        break;
      case '<':
        res = (val_l < val_r);
        break;
      case '=':
        res = (val_l == val_r);
        break;
      case '!':
        res = (val_l != val_r);
        break;
      case 'G':
        res = (val_l >= val_r);
        break;
      case 'L':
        res = (val_l <= val_r);
        break;
    }
    ast_free(l);
    ast_free(r);
    ast *n = ast_allocate(NODE_VALUE);
    n->data.val = res;
    n->res_type = TYPE_BOOL;
    return n;
  }
#endif
  ast *n = ast_allocate(NODE_COMP);
  n->data.op = op;
  n->b1 = l;
  n->b2 = r;
  n->res_type = TYPE_BOOL;
  return n;
}

ast *ast_create_assign(const char *name, ast *expr) {
  var_t *v = hashtable_search(ht, name);
  if (v != nullptr) {
    int v_type = var_get_type(v);
    if (var_get_assign_count(v) > 0 && v_type != expr->res_type) {
      fprintf(stderr, ANSI_RED "[Erreur de Type] : variable '%s'\n" ANSI_RESET,
          name);
      exit(EXIT_FAILURE);
    }
  }
  ast *n = ast_allocate(NODE_ASSIGN);
  n->data.name = strdup(name);
  n->b1 = expr;
  if (v != nullptr) {
    n->res_type = var_get_type(v);
    n->ctx.offset = var_get_offset(v);
  }
  return n;
}

ast *ast_create_if(ast *cond, ast *then_b, ast *else_b) {
  if (cond == nullptr || cond->res_type != TYPE_BOOL) {
    fprintf(stderr, ANSI_RED "[Erreur] : ");
    fprintf(stderr,
        "La condition du IF doit être un BOOL. (Type reçu : %s)\n "ANSI_RESET,
        (cond != nullptr && cond->res_type == TYPE_INT ? "INT" : "UNKNOWN"));
    exit(EXIT_FAILURE);
  }
#ifdef OPTIMIZE
  if (cond->type == NODE_VALUE) {
    bool v = (cond->data.val != 0);
    ast *keep = v ? then_b : else_b;
    ast *drop = v ? else_b : then_b;
    ast_free(cond);
    ast_free(drop);
    return keep;
  }
#endif
  ast *n = ast_allocate(NODE_IF);
  n->b1 = cond;
  n->b2 = then_b;
  n->b3 = else_b;
  return n;
}

ast *ast_create_for(const char *id, ast *start, ast *end, ast *body) {
  var_t *v = hashtable_search(ht, id);
  if (v == nullptr) {
    v = var_empty();
    var_set_type(v, TYPE_INT);
    hashtable_add(ht, id, v);
  }
  var_inc_assign_count(v);
#ifdef OPTIMIZE
  if (start && end && start->type == NODE_VALUE && end->type == NODE_VALUE) {
    if (start->data.val > end->data.val) {
      ast_free(start);
      ast_free(end);
      ast_free(body);
      return nullptr;
    }
  }
#endif
  ast *n = ast_allocate(NODE_FOR);
  n->data.name = strdup(id);
  n->b1 = start;
  n->b2 = end;
  n->b3 = body;
  n->ctx.offset = var_get_offset(v);
  return n;
}

int depends_on_body(ast *assign, ast *body) {
  if (assign == nullptr || body == nullptr) {
    return 0;
  }
  if (assign->type != NODE_ASSIGN) {
    return 0;
  }
  var_t *v = hashtable_search(ht, assign->data.name);
  if (v == nullptr) {
    return 0;
  }
  ast *curr = body;
  while (curr != nullptr) {
    if (curr->type == NODE_ASSIGN) {
      var_t *bv = hashtable_search(ft, curr->data.name);
      if (bv && var_get_offset(bv) == var_get_offset(v)) {
        return 1;
      }
    }
    curr = curr->next;
  }
  return 0;
}

ast *ast_create_while(ast *cond, ast *body) {
  if (cond == nullptr || cond->res_type != TYPE_BOOL) {
    fprintf(stderr, "Erreur : La condition du WHILE doit être un BOOL.\n");
    exit(EXIT_FAILURE);
  }
#ifdef OPTIMIZE
  if (cond->type == NODE_VALUE && cond->data.val == 0) {
    ast_free(cond);
    ast_free(body);
    return nullptr;
  }
#endif
  ast *n = ast_allocate(NODE_WHILE);
  n->b1 = cond;
  n->b2 = body;
  return n;
}

#ifdef OPTIMIZE

static ast *ast_clone(ast *n) {
  if (n == nullptr) {
    return nullptr;
  }
  ast *clone = ast_allocate(n->type);
  clone->res_type = n->res_type;
  clone->ctx = n->ctx;
  if ((n->type == NODE_ID || n->type == NODE_STRING || n->type == NODE_ASSIGN
      || n->type == NODE_FOR || n->type == NODE_CALL || n->type == NODE_FUNC)
      && n->data.name != nullptr) {
    clone->data.name = strdup(n->data.name);
  } else {
    clone->data = n->data;
  }
  clone->b1 = ast_clone(n->b1);
  clone->b2 = ast_clone(n->b2);
  clone->b3 = ast_clone(n->b3);
  clone->next = ast_clone(n->next);
  return clone;
}

static void ast_substitute_arg(ast *clone, ast *arg, int offset) {
  if (clone == nullptr || arg == nullptr) {
    return;
  }
  if (clone->type == NODE_ID && clone->ctx.offset == offset
      && arg->type == NODE_VALUE) {
    if (clone->data.name != nullptr) {
      free(clone->data.name);
      clone->data.name = nullptr;
    }
    clone->type = NODE_VALUE;
    clone->data.val = arg->data.val;
  }
  ast_substitute_arg(clone->b1, arg, offset);
  ast_substitute_arg(clone->b2, arg, offset);
  ast_substitute_arg(clone->b3, arg, offset);
  ast_substitute_arg(clone->next, arg, offset);
}

#endif

ast *ast_create_call(const char *name, ast *args) {
  if (strcmp(name, "printd") == 0 || strcmp(name, "prints") == 0) {
    ast *n = ast_allocate(NODE_CALL);
    n->data.name = strdup(name);
    n->b1 = args;
    return n;
  }
  func_info_t *f = hashtable_search(ft, name);
  if (f == nullptr) {
    exit(1);
  }
#ifdef OPTIMIZE
  ast *body = (ast *) func_get_body(f);
  bool all_literal_args = true;
  int arg_count = 0;
  for (ast *arg = args; arg != nullptr; arg = arg->next) {
    arg_count++;
    if (arg->type != NODE_VALUE) {
      all_literal_args = false;
    }
  }
  if (body != nullptr
      && body->type == NODE_RETURN
      && body->next == nullptr
      && body->b1 != nullptr
      && all_literal_args
      && arg_count == func_get_param_count(f)) {
    ast *inlined_code = ast_clone(body->b1);
    int offset = 0;
    for (ast *arg = args; arg != nullptr; arg = arg->next) {
      ast_substitute_arg(inlined_code, arg, offset);
      offset++;
    }
    inlined_code = ast_reduce(inlined_code, nullptr);
    ast_free(args);
    return inlined_code;
  }
#endif
  func_set_used(f, 1);
  ast *n = ast_allocate(NODE_CALL);
  n->data.name = strdup(name);
  n->b1 = args;
  n->res_type = func_get_return_type(f);
  return n;
}

ast *ast_create_return(ast *expr) {
  ast *n = ast_allocate(NODE_RETURN);
  n->b1 = expr;
  return n;
}

ast *ast_create_func(const char *name, ast *body) {
  ast *n = ast_allocate(NODE_FUNC);
  n->data.name = strdup(name);
  n->b1 = body;
  func_info_t *f_info = hashtable_search(ft, name);
  if (f_info != nullptr) {
    func_set_body(f_info, body);
    func_set_used(f_info, 0);
  }
  return n;
}

int has_guaranteed_return(ast *n) {
  if (n == nullptr) {
    return 0;
  }
  ast *curr = n;
  while (curr != nullptr) {
    if (curr->type == NODE_RETURN) {
      return 1;
    }
    if (curr->type == NODE_IF) {
      int ret_then = (curr->b2
          != nullptr) ? has_guaranteed_return(curr->b2) : 0;
      int ret_else = (curr->b3
          != nullptr) ? has_guaranteed_return(curr->b3) : 0;
      if (ret_then && ret_else) {
        return 1;
      }
    }
    curr = curr->next;
  }
  return 0;
}

ast *ast_link_nodes(ast *h, ast *t) {
  if (h == nullptr) {
    return t;
  }
  if (t == nullptr) {
    return h;
  }
  if (has_guaranteed_return(h)) {
    fprintf(stderr,
        OP_COLOR "Code unreachable détecté et supprimé de l'asm.\n" ANSI_RESET);
    ast_free(t);
    return h;
  }
  ast *curr = h;
  while (curr->next != nullptr) {
    curr = curr->next;
  }
  curr->next = t;
  return h;
}

void ast_generate_asm(ast *n) {
  if (n == nullptr) {
    return;
  }
  int id_label;
  func_info_t *f_info = nullptr;
  if (current_func_name) {
    f_info = hashtable_search(ft, current_func_name);
  }
  switch (n->type) {
    case NODE_VALUE:
      {
        {
          {
            {
              printf("\tconst ax,%d\n", n->data.val);
              printf("\tpush ax\n");
              break;
            }
          }
        }
      }
    case NODE_ID:
      {
        {
          {
            {
              printf("\tcp ax,bp\n");
              int d_id = 0;
              int p_count = func_get_param_count(f_info);
              int l_count = func_get_local_count(f_info);
              if (n->ctx.offset < p_count) {
                d_id = 4 + ((p_count - 1 - n->ctx.offset) * 2);
              } else {
                int loc_idx = n->ctx.offset - p_count;
                d_id = 4 + (p_count * 2)
                    + ((l_count - 1 - loc_idx) * 2);
              }
              printf("\tconst bx,%d\n", d_id);
              printf("\tsub ax,bx\n");
              printf("\tloadw bx,ax\n");
              printf("\tpush bx\n");
              break;
            }
          }
        }
      }
    case NODE_ASSIGN:
      {
        {
          {
            {
              ast_generate_asm(n->b1);
              printf("\tcp ax,bp\n");
              int d_as = 0;
              int p_count = func_get_param_count(f_info);
              int l_count = func_get_local_count(f_info);
              if (n->ctx.offset < p_count) {
                d_as = 4 + ((p_count - 1 - n->ctx.offset) * 2);
              } else {
                int loc_idx = n->ctx.offset - p_count;
                d_as = 4 + (p_count * 2)
                    + ((l_count - 1 - loc_idx) * 2);
              }
              printf("\tconst bx,%d\n", d_as);
              printf("\tsub ax,bx\n");
              printf("\tpop bx\n");
              printf("\tstorew bx,ax\n");
              break;
            }
          }
        }
      }
    case NODE_FUNC:
      {
        {
          {
            {
#ifdef OPTIMIZE
              func_info_t *f = hashtable_search(ft, n->data.name);
              if (f != nullptr && func_get_used(f) == 0 && strcmp(n->data.name,
                    "main_-_-_-_-_-_-_-_-_-_-_-_-_-_-_-__-_-_-_-_-_-_-_-_-_-_-_-_")
                  != 0) {
                if (n->next != nullptr) {
                  ast_generate_asm(n->next);
                }
                return;
              }
#endif
              current_func_name = n->data.name;
              printf(":%s\n", n->data.name);
              printf("\tpush bp\n");
              printf("\tcp bp,sp\n");
              ast_generate_asm(n->b1);
              if (!has_guaranteed_return(n->b1)) {
                printf("\tpop bp\n");
                printf("\tret\n");
              }
              break;
            }
          }
        }
      }
    case NODE_RETURN:
      {
        {
          {
            {
#ifdef OPTIMIZE
              if (n->b1 && n->b1->type == NODE_VALUE) {
                printf("\tconst dx,%d\n", n->b1->data.val);
              } else {
                ast_generate_asm(n->b1);
                printf("\tpop dx\n");
              }
#else
              ast_generate_asm(n->b1);
              printf("\tpop dx\n");
#endif
              printf("\tcp ax,bp\n");
              int p_count = func_get_param_count(f_info);
              int l_count = func_get_local_count(f_info);
              int total_vars = p_count + l_count;
              int dist_res = 4 + (total_vars * 2);
              printf("\tconst bx,%d\n", dist_res);
              printf("\tsub ax,bx\n");
              printf("\tstorew dx,ax\n");
              printf("\tpop bp\n");
              printf("\tret\n");
              break;
            }
          }
        }
      }
    case NODE_OP:
      {
        {
          {
            {
              ast_generate_asm(n->b1);
              ast_generate_asm(n->b2);
              printf("\tpop bx\n");
              printf("\tpop ax\n");
              switch (n->data.op) {
                case '+':
                  printf("\tadd ax,bx\n");
                  break;
                case '-':
                  printf("\tsub ax,bx\n");
                  break;
                case '*':
                  printf("\tmul ax,bx\n");
                  break;
                case '/':
                  printf("\tdiv ax,bx\n");
                  break;
                default:
                  fprintf(stderr, "Erreur : Opérateur '%c' inconnu.\n",
                      n->data.op);
                  break;
              }
              printf("\tpush ax\n");
              break;
            }
          }
        }
      }
    case NODE_COMP:
      {
        {
          {
            {
              ast_generate_asm(n->b1);
              ast_generate_asm(n->b2);
              printf("\tpop bx\n");
              printf("\tpop ax\n");
              int l_true = new_num();
              int l_end = new_num();
              printf("\tconst cx,cmp_true_%d\n", l_true);
              switch (n->data.op) {
                case '<':
                  printf("\tsless ax,bx\n\tjmpc cx\n");
                  break;
                case '>':
                  printf("\tsless bx,ax\n\tjmpc cx\n");
                  break;
                case '=':
                  printf("\tcmp ax,bx\n\tjmpc cx\n");
                  break;
                case '!':
                  printf("\tsless ax,bx\n\tjmpc cx\n");
                  printf("\tsless bx,ax\n\tjmpc cx\n");
                  break;
                case 'L':
                  printf("\tsless ax,bx\n\tjmpc cx\n");
                  printf("\tcmp ax,bx\n\tjmpc cx\n");
                  break;
                case 'G':
                  printf("\tsless bx,ax\n\tjmpc cx\n");
                  printf("\tcmp ax,bx\n\tjmpc cx\n");
                  break;
                default:
                  fprintf(stderr, "Comparaison inconnue : '%c'\n", n->data.op);
                  exit(EXIT_FAILURE);
                  break;
              }
              printf("\tconst ax,0\n");
              printf("\tconst cx,cmp_end_%d\n", l_end);
              printf("\tjmp cx\n");
              printf(":cmp_true_%d\n", l_true);
              printf("\tconst ax,1\n");
              printf(":cmp_end_%d\n", l_end);
              printf("\tpush ax\n");
              break;
            }
          }
        }
      }
    case NODE_IF:
      {
        {
          {
            {
              id_label = new_num();
              ast_generate_asm(n->b1);
              printf("\tpop ax\n");
              printf("\tconst bx,0\n");
              printf("\tconst cx,else_%d\n", id_label);
              printf("\tcmp ax,bx\n");
              printf("\tjmpz cx\n");
              ast_generate_asm(n->b2);
              printf("\tconst cx,end_if_%d\n", id_label);
              printf("\tjmp cx\n");
              printf(":else_%d\n", id_label);
              if (n->b3) {
                ast_generate_asm(n->b3);
              }
              printf(":end_if_%d\n", id_label);
              break;
            }
          }
        }
      }
    case NODE_FOR:
      {
        {
          {
            {
              id_label = new_num();
              int d_for = 0;
              int p_count = func_get_param_count(f_info);
              int l_count = func_get_local_count(f_info);
              if (n->ctx.offset < p_count) {
                d_for = 4 + ((p_count - 1 - n->ctx.offset) * 2);
              } else {
                int loc_idx = n->ctx.offset - p_count;
                d_for = 4 + (p_count * 2)
                    + ((l_count - 1 - loc_idx) * 2);
              }
              ast_generate_asm(n->b1);
              printf("\tcp ax,bp\n");
              printf("\tconst bx,%d\n", d_for);
              printf("\tsub ax,bx\n");
              printf("\tpop bx\n");
              printf("\tstorew bx,ax\n");
              ast_generate_asm(n->b2);
              printf(":for_loop_%d\n", id_label);
              printf("\tcp ax,bp\n");
              printf("\tconst bx,%d\n", d_for);
              printf("\tsub ax,bx\n");
              printf("\tloadw ax,ax\n");
              printf("\tpop bx\n\tpush bx\n");
              printf("\tconst cx,for_end_%d\n", id_label);
              printf("\tsless bx,ax\n");
              printf("\tjmpc cx\n");
              ast_generate_asm(n->b3);
              printf("\tcp ax,bp\n");
              printf("\tconst bx,%d\n", d_for);
              printf("\tsub ax,bx\n");
              printf("\tloadw cx,ax\n");
              printf("\tconst dx,1\n");
              printf("\tadd cx,dx\n");
              printf("\tstorew cx,ax\n");
              printf("\tconst cx,for_loop_%d\n", id_label);
              printf("\tjmp cx\n");
              printf(":for_end_%d\n", id_label);
              printf("\tpop ax\n");
              break;
            }
          }
        }
      }
    case NODE_WHILE:
      {
        {
          {
            {
              id_label = new_num();
              printf(":while_start_%d\n", id_label);
              ast_generate_asm(n->b1);
              printf("\tpop ax\n");
              printf("\tconst bx,0\n");
              printf("\tconst cx,while_end_%d\n", id_label);
              printf("\tcmp ax,bx\n");
              printf("\tjmpz cx\n");
              ast_generate_asm(n->b2);
              printf("\tconst cx,while_start_%d\n", id_label);
              printf("\tjmp cx\n");
              printf(":while_end_%d\n", id_label);
              break;
            }
          }
        }
      }
    case NODE_CALL:
      {
        {
          {
            {
              if (strcmp(n->data.name, "printd") == 0) {
                ast_generate_asm(n->b1);
                if (n->b1 && n->b1->res_type == TYPE_BOOL) {
                  int id = new_num();
                  printf("\tpop ax\n");
                  printf("\tconst bx,0\n");
                  printf("\tconst cx,is_false_%d\n", id);
                  printf("\tcmp ax,bx\n");
                  printf("\tjmpz cx\n");
                  printf("\tconst ax,str_true\n");
                  printf("\tconst cx,print_end_%d\n", id);
                  printf("\tjmp cx\n");
                  printf(":is_false_%d\n", id);
                  printf("\tconst ax,str_false\n");
                  printf(":print_end_%d\n", id);
                  printf("\tcallprintfs ax\n");
                } else {
                  printf("\tcp ax,sp\n");
                  printf("\tcallprintfd ax\n");
                  printf("\tpop ax\n");
                }
              } else if (strcmp(n->data.name, "prints") == 0) {
                ast_generate_asm(n->b1);
                printf("\tpop ax\n");
                printf("\tcallprintfs ax\n");
              } else {
                func_info_t *fun = hashtable_search(ft, n->data.name);
                int r = 2;
                int l_count_fun = func_get_local_count(fun);
                if (fun && l_count_fun > 0) {
                  r += (l_count_fun * 2);
                }
                printf("\tconst ax,%d\n", r);
                printf("\tadd sp,ax\n");
                int nb_args = 0;
                ast *arg = n->b1;
                while (arg) {
                  nb_args++;
                  arg = arg->next;
                }
                if (n->b1) {
                  ast_generate_asm(n->b1);
                }
                printf("\tconst ax,%s\n", n->data.name);
                printf("\tcall ax\n");
                int r_ = nb_args * 2;
                if (fun) {
                  r_ += (l_count_fun * 2);
                }
                if (r_ > 0) {
                  printf("\tconst ax,%d\n", r_);
                  printf("\tsub sp,ax\n");
                }
              }
              break;
            }
          }
        }
      }
    case NODE_STRING:
      {
        {
          {
            {
              printf("\tconst ax,after_str_%d\n", n->ctx.index);
              printf("\tjmp ax\n");
              printf(":str_%d\n@string %s\n", n->ctx.index, n->data.name);
              printf(":after_str_%d\n", n->ctx.index);
              printf("\tconst ax,str_%d\n", n->ctx.index);
              printf("\tpush ax\n");
              break;
            }
          }
        }
      }
  }
  if (n->next != nullptr) {
    ast_generate_asm(n->next);
  }
}

const char *node_type_to_str(node_t type) {
  switch (type) {
    case NODE_VALUE:
      return "VALUE";
    case NODE_ID:
      return "ID";
    case NODE_OP:
      return "OP";
    case NODE_COMP:
      return "COMP";
    case NODE_ASSIGN:
      return "ASSIGN";
    case NODE_IF:
      return "IF";
    case NODE_WHILE:
      return "WHILE";
    case NODE_FOR:
      return "FOR";
    case NODE_CALL:
      return "CALL";
    case NODE_RETURN:
      return "RETURN";
    case NODE_FUNC:
      return "FUNC";
    case NODE_STRING:
      return "STRING";
    default:
      return "UNKNOWN";
  }
}

#ifdef OPTIMIZE

bool ast_is_used(ast *n, const char *name) {
  if (n == nullptr) {
    return false;
  }
  if (n->type == NODE_ID && strcmp(n->data.name, name) == 0) {
    return true;
  }
  return ast_is_used(n->b1, name) || ast_is_used(n->b2, name)
    || ast_is_used(n->b3, name) || ast_is_used(n->next, name);
}

ast *ast_clean(ast *n) {
  if (n == nullptr) {
    return nullptr;
  }
  n->b1 = ast_clean(n->b1);
  n->b2 = ast_clean(n->b2);
  n->b3 = ast_clean(n->b3);
  n->next = ast_clean(n->next);
  return n;
}

void ast_mark_used(ast *n, void *ht) {
  if (n == nullptr || ht == nullptr) {
    return;
  }
  if (n->type == NODE_ID || n->type == NODE_ASSIGN) {
    var_t *v = hashtable_search(ht, n->data.name);
    if (v) {
      var_set_used(v, true);
    }
  }
  ast_mark_used(n->b1, ht);
  ast_mark_used(n->b2, ht);
  ast_mark_used(n->b3, ht);
  ast_mark_used(n->next, ht);
}

static void update_ast_offsets(ast *n, void *ht, int *next_off, int p_limit) {
  if (n == nullptr || ht == nullptr) {
    return;
  }
  if (n->type == NODE_ID || n->type == NODE_ASSIGN || n->type == NODE_FOR) {
    var_t *v = hashtable_search(ht, n->data.name);
    if (v && var_get_offset(v) >= p_limit) {
      if (var_get_used(v)) {
        var_set_offset(v, *next_off);
        var_set_used(v, false);
        (*next_off)++;
      }
      n->ctx.offset = var_get_offset(v);
    }
  }
  update_ast_offsets(n->b1, ht, next_off, p_limit);
  update_ast_offsets(n->b2, ht, next_off, p_limit);
  update_ast_offsets(n->b3, ht, next_off, p_limit);
  update_ast_offsets(n->next, ht, next_off, p_limit);
}

void ast_recompute_offsets(ast *n) {
  if (n == nullptr || n->type != NODE_FUNC) {
    return;
  }
  void *ht = n->ctx.local_ht;
  func_info_t *f = hashtable_search(ft, n->data.name);
  int p_count = func_get_param_count(f);
  int next_local_offset = p_count;
  ast_mark_used(n->b1, ht);
  update_ast_offsets(n->b1, ht, &next_local_offset, p_count);
  n->ctx.stack_size = next_local_offset * 2;
  func_set_local_count(f, next_local_offset);
}

ast *ast_reduce(ast *n, void *active_ht) {
  if (n == nullptr) {
    return nullptr;
  }
  if (n->type == NODE_FUNC) {
    active_ht = n->ctx.local_ht;
  }
  n->b1 = ast_reduce(n->b1, active_ht);
  n->b2 = ast_reduce(n->b2, active_ht);
  n->b3 = ast_reduce(n->b3, active_ht);
  n->next = ast_reduce(n->next, active_ht);
  if (n->type == NODE_COMP
      && is_constant(n->b1)
      && is_constant(n->b2)) {
    int a = n->b1->data.val;
    int b = n->b2->data.val;
    int v;
    switch (n->data.op) {
      case '<':
        v = (a < b);
        break;
      case '>':
        v = (a > b);
        break;
      case '=':
        v = (a == b);
        break;
      case '!':
        v = (a != b);
        break;
      case 'L':
        v = (a <= b);
        break;
      case 'G':
        v = (a >= b);
        break;
      default:
        fprintf(stderr, "Comparaison inconnue : '%c'\n", n->data.op);
        exit(EXIT_FAILURE);
    }
    ast_free(n->b1);
    ast_free(n->b2);
    n->b1 = nullptr;
    n->b2 = nullptr;
    n->type = NODE_VALUE;
    n->data.val = v;
    n->res_type = TYPE_BOOL;
    tree_changed = true;
    return n;
  }
  if (n->type == NODE_IF
      && is_constant(n->b1)
      && n->b1->res_type == TYPE_BOOL) {
    bool v = (n->b1->data.val != 0);
    ast *keep = v ? n->b2 : n->b3;
    ast *drop = v ? n->b3 : n->b2;
    ast *next = n->next;
    ast_free(n->b1);
    ast_free(drop);
    n->b1 = nullptr;
    n->b2 = nullptr;
    n->b3 = nullptr;
    n->next = nullptr;
    free(n);
    if (keep != nullptr) {
      ast *t = keep;
      while (t->next != nullptr) {
        t = t->next;
      }
      t->next = next;
    } else {
      keep = next;
    }
    tree_changed = true;
    return keep;
  }
  if (n->type == NODE_ID && active_ht != nullptr) {
    var_t *v = hashtable_search(active_ht, n->data.name);
    if (v != nullptr && var_get_constant(v)) {
      int val = var_get_last_value(v);
      free(n->data.name);
      n->type = NODE_VALUE;
      n->data.val = val;
      n->res_type = var_get_type(v);
      tree_changed = true;
      return n;
    }
  }
  if (n->type == NODE_OP
      && is_constant(n->b1)
      && is_constant(n->b2)) {
    int a = n->b1->data.val;
    int b = n->b2->data.val;
    int v;
    switch (n->data.op) {
      case '+':
        v = a + b;
        break;
      case '-':
        v = a - b;
        break;
      case '*':
        v = a * b;
        break;
      case '/':
        if (b == 0) {
          fprintf(stderr,
              ANSI_RED
              "[Erreur] : division par zero dans une expression constante.\n"
              ANSI_RESET);
          exit(EXIT_FAILURE);
        }
        if (a == INT_MIN && b == -1) {
          fprintf(stderr,
              ANSI_RED
              "[Erreur] : debordement dans une division constante.\n"
              ANSI_RESET);
          exit(EXIT_FAILURE);
        }
        v = a / b;
        break;
      default:
        fprintf(stderr,
            ANSI_RED
            "[Erreur] : operateur inconnu '%c'.\n"
            ANSI_RESET,
            n->data.op);
        exit(EXIT_FAILURE);
    }
    ast_free(n->b1);
    ast_free(n->b2);
    n->b1 = nullptr;
    n->b2 = nullptr;
    n->type = NODE_VALUE;
    n->data.val = v;
    n->res_type = TYPE_INT;
    tree_changed = true;
    return n;
  }
  if (n->type == NODE_ASSIGN && is_constant(n->b1) && active_ht != nullptr) {
    var_t *v = hashtable_search(active_ht, n->data.name);
    if (v && var_get_assign_count(v) == 1) {
      if (!var_get_constant(v) || var_get_last_value(v) != n->b1->data.val) {
        var_set_constant(v, true);
        var_set_last_value(v, n->b1->data.val);
        tree_changed = true;
      }
    }
  }
  return n;
}

#endif

void ast_dump_tree(ast *n, int level) {
  if (n == nullptr) {
    return;
  }
  if (n->type == NODE_FUNC) {
    current_func_name = n->data.name;
  }
  if (level > 0) {
    fprintf(stderr, TREE_BRANCH);
    for (int i = 0; i < level; i++) {
      if (i == level - 1) {
        fprintf(stderr, " |--- ");
      } else {
        fprintf(stderr, " |    ");
      }
    }
    fprintf(stderr, ANSI_RESET);
  }
  fprintf(stderr, TYPE_COLOR ANSI_BOLD "%s" ANSI_RESET,
      node_type_to_str(n->type));
  switch (n->type) {
    case NODE_ID:
    case NODE_CALL:
    case NODE_ASSIGN:
      if (n->data.name != nullptr) {
        fprintf(stderr, " : " ID_COLOR "%s" ANSI_RESET, n->data.name);
      }
      break;
    case NODE_FUNC:
      if (n->data.name != nullptr) {
        fprintf(stderr, " : " ANSI_BOLD "%s" ANSI_RESET, n->data.name);
      }
      break;
    case NODE_VALUE:
      fprintf(stderr, " : " VAL_COLOR "%d" ANSI_RESET, n->data.val);
      break;
    case NODE_OP:
    case NODE_COMP:
      fprintf(stderr, " : " OP_COLOR ANSI_BOLD "'%c'" ANSI_RESET, n->data.op);
      break;
    case NODE_STRING:
      fprintf(stderr, " : " STR_COLOR "\"%s\"" ANSI_RESET, n->data.name);
      break;
    default:
      break;
  }
  if (n->type == NODE_ID || n->type == NODE_ASSIGN || n->type == NODE_FOR) {
    int param_count = 0;
    if (current_func_name) {
      func_info_t *f_info = hashtable_search(ft, current_func_name);
      if (f_info) {
        param_count = func_get_param_count(f_info);
      }
    }
    if (n->ctx.offset < param_count) {
      fprintf(stderr, ANSI_PARAM "  <-- [Param, offset = %d]" ANSI_RESET,
          n->ctx.offset);
    } else {
      fprintf(stderr, ANSI_LOCAL "  <-- [Local, offset = %d]" ANSI_RESET,
          n->ctx.offset);
    }
  }
  fprintf(stderr, "\n");
  ast_dump_tree(n->b1, level + 1);
  ast_dump_tree(n->b2, level + 1);
  ast_dump_tree(n->b3, level + 1);
  if (n->next) {
    if (level == 0) {
      fprintf(stderr, "\n");
    }
    ast_dump_tree(n->next, level);
  }
}
