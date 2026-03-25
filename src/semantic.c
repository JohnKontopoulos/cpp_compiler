#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h"

int sem_error_count = 0;

void sem_error(const char *msg, const char *name)
{
    fprintf(stderr, "Semantic error: %s '%s'\n", msg, name ? name : "");
    sem_error_count++;
}

void sem_warning(const char *msg, const char *name)
{
    fprintf(stderr, "Semantic warning: %s '%s'\n", msg, name ? name : "");
}

int types_compatible(SymType t1, SymType t2)
{
    if (t1 == t2)
        return 1;
    /* αριθμητικοί τύποι είναι συμβατοί μεταξύ τους */
    if ((t1 == TYPE_INT || t1 == TYPE_FLOAT) &&
        (t2 == TYPE_INT || t2 == TYPE_FLOAT))
        return 1;
    return 0;
}

SymType result_type(SymType t1, SymType t2)
{
    if (t1 == TYPE_FLOAT || t2 == TYPE_FLOAT)
        return TYPE_FLOAT;
    if (t1 == TYPE_INT || t2 == TYPE_INT)
        return TYPE_INT;
    return t1;
}

SymType semantic_check_expr(ASTNode *node, SymTable *st)
{
    if (!node)
        return TYPE_UNKNOWN;

    switch (node->kind)
    {
    case NODE_ICONST:
        node->type = TYPE_INT;
        return TYPE_INT;

    case NODE_FCONST:
        node->type = TYPE_FLOAT;
        return TYPE_FLOAT;

    case NODE_CCONST:
        node->type = TYPE_CHAR;
        return TYPE_CHAR;

    case NODE_SCONST:
        node->type = TYPE_STRING;
        return TYPE_STRING;

    case NODE_ID:
    {
        Symbol *s = symtable_lookup(st, node->name);
        if (!s)
        {
            sem_error("undeclared identifier", node->name);
            node->type = TYPE_UNKNOWN;
            return TYPE_UNKNOWN;
        }
        node->type = s->type;
        return s->type;
    }

    case NODE_BINOP:
    {
        SymType lt = semantic_check_expr(node->left, st);
        SymType rt = semantic_check_expr(node->right, st);

        /* Λογικοί τελεστές */
        if (strcmp(node->name, "||") == 0 ||
            strcmp(node->name, "&&") == 0)
        {
            if (lt != TYPE_INT)
                sem_error("logical operator requires int", node->name);
            if (rt != TYPE_INT)
                sem_error("logical operator requires int", node->name);
            node->type = TYPE_INT;
            return TYPE_INT;
        }

        /* Τελεστές σύγκρισης */
        if (strcmp(node->name, "==") == 0 ||
            strcmp(node->name, "<>") == 0)
        {
            if (!types_compatible(lt, rt))
                sem_error("incompatible types in comparison", node->name);
            node->type = TYPE_INT;
            return TYPE_INT;
        }

        /* Αριθμητικοί τελεστές */
        if (!types_compatible(lt, rt))
            sem_error("incompatible types in expression", node->name);
        node->type = result_type(lt, rt);
        return node->type;
    }

    case NODE_UNOP:
    {
        SymType t = semantic_check_expr(node->left, st);
        if (strcmp(node->name, "!") == 0)
        {
            if (t != TYPE_INT)
                sem_error("logical not requires int", "!");
            node->type = TYPE_INT;
            return TYPE_INT;
        }
        node->type = t;
        return t;
    }

    case NODE_ASSIGN:
    {
        SymType lt = semantic_check_expr(node->left, st);
        SymType rt = semantic_check_expr(node->right, st);
        if (!types_compatible(lt, rt))
            sem_error("incompatible types in assignment", "=");
        node->type = lt;
        return lt;
    }

    case NODE_CALL:
    {
        Symbol *s = symtable_lookup(st, node->name);
        if (!s)
        {
            sem_error("undeclared function", node->name);
            node->type = TYPE_UNKNOWN;
            return TYPE_UNKNOWN;
        }
        if (s->kind != SYM_FUNCTION)
            sem_error("not a function", node->name);
        node->type = s->type;
        return s->type;
    }

    default:
        return TYPE_UNKNOWN;
    }
}

void semantic_check_stmt(ASTNode *node, SymTable *st)
{
    if (!node)
        return;

    switch (node->kind)
    {
    case NODE_ASSIGN:
    case NODE_BINOP:
    case NODE_UNOP:
    case NODE_CALL:
    case NODE_ID:
    case NODE_ICONST:
    case NODE_FCONST:
    case NODE_CCONST:
    case NODE_SCONST:
        semantic_check_expr(node, st);
        break;

    case NODE_IF:
    {
        SymType ct = semantic_check_expr(node->left, st);
        if (ct != TYPE_INT && ct != TYPE_UNKNOWN)
            sem_error("if condition must be int", "if");
        semantic_check_stmt(node->right, st);
        if (node->extra)
            semantic_check_stmt(node->extra, st);
        break;
    }

    case NODE_WHILE:
    {
        SymType ct = semantic_check_expr(node->left, st);
        if (ct != TYPE_INT && ct != TYPE_UNKNOWN)
            sem_error("while condition must be int", "while");
        semantic_check_stmt(node->right, st);
        break;
    }

    case NODE_FOR:
        semantic_check_expr(node->left, st);
        semantic_check_expr(node->right, st);
        semantic_check_expr(node->extra, st);
        semantic_check_stmt(node->next, st);
        break;

    case NODE_RETURN:
        semantic_check_expr(node->left, st);
        break;

    case NODE_COMPOUND:
    {
        ASTNode *stmt = node->left;
        while (stmt)
        {
            semantic_check_stmt(stmt, st);
            stmt = stmt->next;
        }
        break;
    }

    case NODE_COUT:
    {
        ASTNode *item = node->left;
        while (item)
        {
            semantic_check_expr(item, st);
            item = item->next;
        }
        break;
    }

    case NODE_CIN:
    {
        ASTNode *item = node->left;
        while (item)
        {
            semantic_check_expr(item, st);
            item = item->next;
        }
        break;
    }

    default:
        break;
    }
}

void semantic_check_program(ASTNode *root, SymTable *st)
{
    if (!root)
        return;
    printf("\n========== Semantic Analysis ==========\n");
    semantic_check_stmt(root, st);
    if (sem_error_count == 0)
        printf("No semantic errors found!\n");
    else
        printf("%d semantic error(s) found.\n", sem_error_count);
}