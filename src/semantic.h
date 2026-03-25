#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "symtable.h"
#include "ast.h"

/* Έλεγχος τύπων */
SymType semantic_check_expr(ASTNode *node, SymTable *st);
void semantic_check_stmt(ASTNode *node, SymTable *st);
void semantic_check_program(ASTNode *root, SymTable *st);

/* Συμβατότητα τύπων */
int types_compatible(SymType t1, SymType t2);
SymType result_type(SymType t1, SymType t2);

/* Μηνύματα σφαλμάτων */
void sem_error(const char *msg, const char *name);
void sem_warning(const char *msg, const char *name);

extern int sem_error_count;

#endif