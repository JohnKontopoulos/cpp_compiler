#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "ast.h"

/* Βελτιστοποίηση ΑΣΔ */
ASTNode *optimize(ASTNode *root);

/* #1 Αποτίμηση σταθερών */
ASTNode *constant_folding(ASTNode *node);

/* #3 Απαλοιφή άχρηστου κώδικα */
ASTNode *dead_code_elimination(ASTNode *node);

#endif