//  AST : partie interface pour la spécification d'un arbre syntaxique abstraite
// (Abstract - Syntaxic - Tree ).

#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>

#include "commun.h"
#include "hashtable.h"

extern bool tree_changed;

// Définition des types de nœuds possibles dans l'AST
typedef enum {
  NODE_VALUE,    // Une valeur numérique (ex: 2, 84)
  NODE_ID,       // Un identifiant (nom de variable)
  NODE_OP,       // Une opération binaire ou unaire (ex: +, -, *)
  NODE_COMP,     // Une comparaison (ex: <, >, ==)
  NODE_ASSIGN,   // Une assignation (ex: \SET{x} {y})
  NODE_IF,       // Une structure conditionnelle if
  NODE_WHILE,    // Une boucle while
  NODE_FOR,      // Une boucle for
  NODE_CALL,     // Un appel de fonction ou affichage
  NODE_RETURN,   // Une instruction return
  NODE_FUNC,     // Définition de fonction
  NODE_STRING    // Une chaîne de caractères
} node_t;

typedef struct ast {
  node_t type;       // Type de nœud
  int line;
  type_s res_type;   // Type du résultat de ce nœud (INT_TYPE, BOOL_TYPE)

  union {
    int val;
    char *name;
    char op;
  } data;

  struct {
    int label;
    int offset;
    int argc;
    int stack_size;
    int is_param;
    int index;
    int total_locals;
    void *local_ht;
  } ctx;

  struct ast *b1;   // Branche principale / gauche / condition
  struct ast *b2;   // Branche secondaire / droite / corps
  struct ast *b3;   // Troisième branche (ex: else ou troisième partie de for)
  struct ast *b4;   // Quatrième branche (NON utilisée...)
  struct ast *next; // Nœud suivant
} ast;

//  ast_create_value : crée un nœud représentant une valeur entière dans l'AST
extern ast *ast_create_value(int v);

//  ast_create_id : crée un nœud représentant une variable (identifiant) dans
//    l'AST.
extern ast *ast_create_id(const char *name);

//  ast_create_op : crée un nœud représentant une opération binaire (+, -, *, /,
//    etc.) avec ses deux opérandes left and right.
extern ast *ast_create_op(char op, ast *l, ast *r);

//  ast_create_comp : crée un nœud représentant une comparaison (<, >, ==, !=,
//    etc.) entre deux expressions.
extern ast *ast_create_comp(char op, ast *l, ast *r);

//  ast_create_assign : crée un nœud représentant une affectation (Var = expr).
extern ast *ast_create_assign(const char *name, ast *expr);

//  ast_create_if : crée un nœud représentant une instruction conditionnelle if
//    (cond) then else.
extern ast *ast_create_if(ast *cond, ast *then_b, ast *else_b);

//  ast_create_while : crée un nœud représentant une boucle
//    while (cond) { body }.
extern ast *ast_create_while(ast *cond, ast *body);

//  ast_create_for : crée un nœud représentant une boucle for
//    (id = start; id <= end; id++) { body }.
extern ast *ast_create_for(const char *id, ast *start, ast *end, ast *body);

//  ast_create_call : crée un nœud représentant un appel de fonction avec ses
//    arguments.
extern ast *ast_create_call(const char *name, ast *args);

//  ast_create_return : crée un nœud représentant un return d’une expression.
extern ast *ast_create_return(ast *expr);

//  ast_create_func : crée un noeud représentant une fonction avec un nom de la
//    fonction  et un corps (liste d’instructions).
extern ast *ast_create_func(const char *name, ast *body);

//  ast_create_string : crée un nœud représentant une chaîne de caractères
//    littérale pointée par str dans l’AST.
extern ast *ast_create_string(char *str);

//  ast_link_nodes : relie deux noeuds d’instructions de l’AST en un seul noeud.
//    Si h ou t est nul, renvoie l’autre nœud.
//    Si `h` contient un return garanti, t est considéré comme inatteignable et
//    libéré. Sinon, parcourt h jusqu’au dernier nœud et met t dans next du
//    dernier nœud. Retourne la tête h de la séquence combinée.
extern ast *ast_link_nodes(ast *h, ast *t);

#ifdef OPTIMIZE

//  ast_reduce : Fonction unique pour plier les constantes et propager les
//    variables.
extern ast *ast_reduce(ast *n, void *current_scope_ht);

#endif

//  ast_clean : simplifie ou nettoie l’AST pointé par n en supprimant les nœuds
//    inutiles, par exemple les branches mortes ou instructions redondantes.
//    Retourne le noued racine nettoyée de l’AST.
extern ast *ast_clean(ast *n);

//  ast_recompute_offsets : Re-calcule la taille de la pile et compacte les
//     offsets.
extern void ast_recompute_offsets(ast *n);

//  ast_optimize_full : dernière fonction d'optimisation de l'AST.
extern void ast_optimize_full(ast **ptr);

//  ast_generate_asm : parcourt l’AST pointé par n et génère le code assembleur
//    correspondant aux instructions et expressions qu’il contient.
extern void ast_generate_asm(ast *n);

//  ast_dump_tree : affiche l’AST pointé par n sous forme arborescente indentée,
//    ou 'level' indique le niveau d’indentation pour les sous-nœuds.
extern void ast_dump_tree(ast *n, int level);

//  count_locals : retourne le nombre de variables locales dans l'AST pointé par
//    n.
extern int count_locals(ast *n);

//  has_guaranteed_return : Vérifie que l'algo defini contient bien un return.
extern int has_guaranteed_return(ast *n);

#endif
