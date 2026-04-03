#ifndef AST_H
#define AST_H

#include "symtable.h"

/* Τύποι κόμβων ΑΣΔ */
typedef enum
{
    /* Εκφράσεις */
    NODE_ICONST,
    NODE_FCONST,
    NODE_CCONST,
    NODE_SCONST,
    NODE_ID,
    NODE_BINOP,
    NODE_UNOP,
    NODE_ASSIGN,
    NODE_CALL,
    NODE_INDEX,
    NODE_FIELD,
    NODE_INCDEC,
    NODE_LIST,
    NODE_CAST, /* μετατροπή τύπου */

    /* Εντολές */
    NODE_EXPR_STMT,
    NODE_IF,
    NODE_WHILE,
    NODE_FOR,
    NODE_RETURN,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_COMPOUND,
    NODE_CIN,
    NODE_COUT,

    /* Δηλώσεις */
    NODE_VAR_DECL,
    NODE_FUNC_DECL,
    NODE_PARAM,

    /* Πρόγραμμα */
    NODE_PROGRAM,
    NODE_FUNC_DEF
} NodeKind;

/* Κόμβος ΑΣΔ */
typedef struct ASTNode
{
    NodeKind kind;
    SymType type;
    int is_lvalue;

    union
    {
        int ival;
        float fval;
        char cval;
        char *sval;
        char op;
    } val;

    char *name;

    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *extra;
    struct ASTNode *next;
} ASTNode;

/* Δημιουργία κόμβων */
ASTNode *ast_make_iconst(int val);
ASTNode *ast_make_fconst(float val);
ASTNode *ast_make_cconst(char val);
ASTNode *ast_make_sconst(char *val);
ASTNode *ast_make_id(char *name, SymType type);
ASTNode *ast_make_binop(char *op, ASTNode *left, ASTNode *right);
ASTNode *ast_make_unop(char *op, ASTNode *operand);
ASTNode *ast_make_assign(ASTNode *left, ASTNode *right);
ASTNode *ast_make_call(char *name, ASTNode *args);
ASTNode *ast_make_if(ASTNode *cond, ASTNode *then_br, ASTNode *else_br);
ASTNode *ast_make_while(ASTNode *cond, ASTNode *body);
ASTNode *ast_make_for(ASTNode *init, ASTNode *cond, ASTNode *step, ASTNode *body);
ASTNode *ast_make_return(ASTNode *expr);
ASTNode *ast_make_compound(ASTNode *stmts);
ASTNode *ast_make_var_decl(char *name, SymType type);
ASTNode *ast_make_cout(ASTNode *exprs);
ASTNode *ast_make_cin(ASTNode *vars);
ASTNode *ast_make_node_simple(NodeKind kind);
ASTNode *ast_make_cast(ASTNode *expr, SymType to_type);
ASTNode *ast_append(ASTNode *list, ASTNode *node);

/* Εκτύπωση ΑΣΔ */
void ast_print(ASTNode *node, int indent);

/* Απελευθέρωση μνήμης */
void ast_free(ASTNode *node);

#endif