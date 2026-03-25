#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

static ASTNode *ast_make_node(NodeKind kind)
{
    ASTNode *n = (ASTNode *)calloc(1, sizeof(ASTNode));
    n->kind = kind;
    n->type = TYPE_UNKNOWN;
    return n;
}

ASTNode *ast_make_iconst(int val)
{
    ASTNode *n = ast_make_node(NODE_ICONST);
    n->val.ival = val;
    n->type = TYPE_INT;
    return n;
}

ASTNode *ast_make_fconst(float val)
{
    ASTNode *n = ast_make_node(NODE_FCONST);
    n->val.fval = val;
    n->type = TYPE_FLOAT;
    return n;
}

ASTNode *ast_make_cconst(char val)
{
    ASTNode *n = ast_make_node(NODE_CCONST);
    n->val.cval = val;
    n->type = TYPE_CHAR;
    return n;
}

ASTNode *ast_make_sconst(char *val)
{
    ASTNode *n = ast_make_node(NODE_SCONST);
    n->val.sval = strdup(val);
    n->type = TYPE_STRING;
    return n;
}

ASTNode *ast_make_id(char *name, SymType type)
{
    ASTNode *n = ast_make_node(NODE_ID);
    n->name = strdup(name);
    n->type = type;
    n->is_lvalue = 1;
    return n;
}

ASTNode *ast_make_binop(char *op, ASTNode *left, ASTNode *right)
{
    ASTNode *n = ast_make_node(NODE_BINOP);
    n->name = strdup(op);
    n->left = left;
    n->right = right;
    if (left && right)
    {
        if (left->type == TYPE_FLOAT || right->type == TYPE_FLOAT)
            n->type = TYPE_FLOAT;
        else
            n->type = left->type;
    }
    return n;
}

ASTNode *ast_make_unop(char *op, ASTNode *operand)
{
    ASTNode *n = ast_make_node(NODE_UNOP);
    n->name = strdup(op);
    n->left = operand;
    if (operand)
        n->type = operand->type;
    return n;
}

ASTNode *ast_make_assign(ASTNode *left, ASTNode *right)
{
    ASTNode *n = ast_make_node(NODE_ASSIGN);
    n->left = left;
    n->right = right;
    if (left)
        n->type = left->type;
    return n;
}

ASTNode *ast_make_call(char *name, ASTNode *args)
{
    ASTNode *n = ast_make_node(NODE_CALL);
    if (name)
        n->name = strdup(name);
    n->left = args;
    return n;
}

ASTNode *ast_make_if(ASTNode *cond, ASTNode *then_br, ASTNode *else_br)
{
    ASTNode *n = ast_make_node(NODE_IF);
    n->left = cond;
    n->right = then_br;
    n->extra = else_br;
    return n;
}

ASTNode *ast_make_while(ASTNode *cond, ASTNode *body)
{
    ASTNode *n = ast_make_node(NODE_WHILE);
    n->left = cond;
    n->right = body;
    return n;
}

ASTNode *ast_make_for(ASTNode *init, ASTNode *cond, ASTNode *step, ASTNode *body)
{
    ASTNode *n = ast_make_node(NODE_FOR);
    n->left = init;
    n->right = cond;
    n->extra = step;
    n->next = body;
    return n;
}

ASTNode *ast_make_return(ASTNode *expr)
{
    ASTNode *n = ast_make_node(NODE_RETURN);
    n->left = expr;
    return n;
}

ASTNode *ast_make_compound(ASTNode *stmts)
{
    ASTNode *n = ast_make_node(NODE_COMPOUND);
    n->left = stmts;
    n->next = NULL;
    return n;
}

ASTNode *ast_make_cout(ASTNode *exprs)
{
    ASTNode *n = ast_make_node(NODE_COUT);
    n->left = exprs;
    return n;
}

ASTNode *ast_make_cin(ASTNode *vars)
{
    ASTNode *n = ast_make_node(NODE_CIN);
    n->left = vars;
    return n;
}

ASTNode *ast_make_node_simple(NodeKind kind)
{
    return ast_make_node(kind);
}

ASTNode *ast_append(ASTNode *list, ASTNode *node)
{
    if (!node)
        return list;
    if (!list)
        return node;

    ASTNode *last = list;
    int count = 0;
    while (last->next != NULL && count < 10000)
    {
        last = last->next;
        count++;
    }
    last->next = node;
    node->next = NULL;
    return list;
}

static const char *nodekind_str(NodeKind k)
{
    switch (k)
    {
    case NODE_ICONST:
        return "ICONST";
    case NODE_FCONST:
        return "FCONST";
    case NODE_CCONST:
        return "CCONST";
    case NODE_SCONST:
        return "SCONST";
    case NODE_ID:
        return "ID";
    case NODE_BINOP:
        return "BINOP";
    case NODE_UNOP:
        return "UNOP";
    case NODE_ASSIGN:
        return "ASSIGN";
    case NODE_CALL:
        return "CALL";
    case NODE_INDEX:
        return "INDEX";
    case NODE_FIELD:
        return "FIELD";
    case NODE_INCDEC:
        return "INCDEC";
    case NODE_IF:
        return "IF";
    case NODE_WHILE:
        return "WHILE";
    case NODE_FOR:
        return "FOR";
    case NODE_RETURN:
        return "RETURN";
    case NODE_BREAK:
        return "BREAK";
    case NODE_CONTINUE:
        return "CONTINUE";
    case NODE_COMPOUND:
        return "COMPOUND";
    case NODE_COUT:
        return "COUT";
    case NODE_CIN:
        return "CIN";
    case NODE_VAR_DECL:
        return "VAR_DECL";
    case NODE_EXPR_STMT:
        return "EXPR_STMT";
    default:
        return "UNKNOWN";
    }
}

static void ast_print_node(ASTNode *node, int indent)
{
    if (!node)
        return;
    if (indent > 30)
        return;

    for (int i = 0; i < indent; i++)
        printf("  ");

    printf("[%s]", nodekind_str(node->kind));

    switch (node->kind)
    {
    case NODE_ICONST:
        printf(" %d", node->val.ival);
        break;
    case NODE_FCONST:
        printf(" %f", node->val.fval);
        break;
    case NODE_CCONST:
        printf(" '%c'", node->val.cval);
        break;
    case NODE_SCONST:
        printf(" \"%s\"", node->val.sval);
        break;
    case NODE_ID:
        printf(" %s", node->name);
        break;
    case NODE_BINOP:
    case NODE_UNOP:
        printf(" %s", node->name);
        break;
    case NODE_CALL:
        printf(" %s()", node->name ? node->name : "?");
        break;
    case NODE_VAR_DECL:
        printf(" %s:%s", node->name, symtype_to_str(node->type));
        break;
    default:
        break;
    }
    printf(" <%s>\n", symtype_to_str(node->type));

    /* Εκτύπωση παιδιών ΜΟΝΟ - ποτέ next */
    ast_print_node(node->left, indent + 1);
    ast_print_node(node->right, indent + 1);
    ast_print_node(node->extra, indent + 1);
}

void ast_print(ASTNode *node, int indent)
{
    if (!node)
        return;

    if (node->kind == NODE_COMPOUND)
    {
        for (int i = 0; i < indent; i++)
            printf("  ");
        printf("[COMPOUND] <unknown>\n");
        ASTNode *stmt = node->left;
        while (stmt)
        {
            ast_print(stmt, indent + 1);
            stmt = stmt->next;
        }
    }
    else
    {
        ast_print_node(node, indent);
    }
}

void ast_free(ASTNode *node)
{
    if (!node)
        return;
    ast_free(node->left);
    ast_free(node->right);
    ast_free(node->extra);
    /* ΜΗΝ κάνεις free το next εδώ - γίνεται από τον caller */
    if (node->name)
        free(node->name);
    if (node->kind == NODE_SCONST && node->val.sval)
        free(node->val.sval);
    free(node);
}