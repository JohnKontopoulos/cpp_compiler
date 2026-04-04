#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "optimizer.h"

/* ==================== #4 Διάδοση Αντιγράφου ==================== */

#define MAX_COPIES 64

typedef struct
{
    char var[64];
    char copy[64];
    int is_const;
    int const_val;
} CopyEntry;

static CopyEntry copy_table[MAX_COPIES];
static int copy_count = 0;

static void copy_table_clear()
{
    copy_count = 0;
}

static void copy_table_add(const char *var, const char *copy,
                           int is_const, int const_val)
{
    for (int i = 0; i < copy_count; i++)
    {
        if (strcmp(copy_table[i].var, var) == 0)
        {
            strncpy(copy_table[i].copy, copy, 63);
            copy_table[i].is_const = is_const;
            copy_table[i].const_val = const_val;
            return;
        }
    }
    if (copy_count >= MAX_COPIES)
        return;
    strncpy(copy_table[copy_count].var, var, 63);
    strncpy(copy_table[copy_count].copy, copy, 63);
    copy_table[copy_count].is_const = is_const;
    copy_table[copy_count].const_val = const_val;
    copy_count++;
}

static void copy_table_invalidate(const char *var)
{
    for (int i = 0; i < copy_count; i++)
    {
        if (strcmp(copy_table[i].var, var) == 0 ||
            strcmp(copy_table[i].copy, var) == 0)
        {
            copy_table[i] = copy_table[--copy_count];
            i--;
        }
    }
}

static CopyEntry *copy_table_lookup(const char *var)
{
    for (int i = 0; i < copy_count; i++)
    {
        if (strcmp(copy_table[i].var, var) == 0)
            return &copy_table[i];
    }
    return NULL;
}

ASTNode *copy_propagation(ASTNode *node)
{
    if (!node)
        return NULL;

    switch (node->kind)
    {

    case NODE_ASSIGN:
    {
        node->right = copy_propagation(node->right);

        if (node->left && node->left->kind == NODE_ID)
        {
            const char *lhs = node->left->name;
            copy_table_invalidate(lhs);

            if (node->right && node->right->kind == NODE_ID)
            {
                fprintf(stderr,
                        "[Optimizer] Copy propagation: %s = %s\n",
                        lhs, node->right->name);
                copy_table_add(lhs, node->right->name, 0, 0);
            }
            else if (node->right && node->right->kind == NODE_ICONST)
            {
                fprintf(stderr,
                        "[Optimizer] Copy propagation: %s = %d\n",
                        lhs, node->right->val.ival);
                copy_table_add(lhs, "", 1, node->right->val.ival);
            }
        }
        return node;
    }

    case NODE_ID:
    {
        CopyEntry *e = copy_table_lookup(node->name);
        if (e)
        {
            if (e->is_const)
            {
                fprintf(stderr,
                        "[Optimizer] Copy propagation: replace %s with %d\n",
                        node->name, e->const_val);
                node->kind = NODE_ICONST;
                node->val.ival = e->const_val;
                node->type = TYPE_INT;
                if (node->name)
                {
                    free(node->name);
                    node->name = NULL;
                }
            }
            else if (strlen(e->copy) > 0)
            {
                fprintf(stderr,
                        "[Optimizer] Copy propagation: replace %s with %s\n",
                        node->name, e->copy);
                free(node->name);
                node->name = strdup(e->copy);
            }
        }
        return node;
    }

    case NODE_IF:
    case NODE_WHILE:
    case NODE_FOR:
    {
        node->left = copy_propagation(node->left);
        copy_table_clear();
        node->right = copy_propagation(node->right);
        node->extra = copy_propagation(node->extra);
        copy_table_clear();
        if (node->next)
            node->next = copy_propagation(node->next);
        return node;
    }

    case NODE_COMPOUND:
    {
        /* Διαπέραση λίστας εντολών μέσω next */
        ASTNode *stmt = node->left;
        ASTNode *prev = NULL;
        ASTNode *first = NULL;

        while (stmt)
        {
            ASTNode *next = stmt->next;
            stmt->next = NULL;

            /* Βελτιστοποίηση τρέχουσας εντολής */
            ASTNode *new_stmt = copy_propagation(stmt);

            if (new_stmt)
            {
                new_stmt->next = NULL;
                if (!first)
                    first = new_stmt;
                if (prev)
                    prev->next = new_stmt;
                prev = new_stmt;
            }

            stmt = next;
        }

        node->left = first;
        if (node->next)
            node->next = copy_propagation(node->next);
        return node;
    }

    default:
    {
        node->left = copy_propagation(node->left);
        node->right = copy_propagation(node->right);
        node->extra = copy_propagation(node->extra);
        if (node->next)
            node->next = copy_propagation(node->next);
        return node;
    }
    }
}

/* ==================== #1 Αποτίμηση Σταθερών ==================== */
ASTNode *constant_folding(ASTNode *node)
{
    if (!node)
        return NULL;

    node->left = constant_folding(node->left);
    node->right = constant_folding(node->right);
    node->extra = constant_folding(node->extra);

    /* Int constant folding */
    if (node->kind == NODE_BINOP &&
        node->left && node->left->kind == NODE_ICONST &&
        node->right && node->right->kind == NODE_ICONST)
    {

        int lval = node->left->val.ival;
        int rval = node->right->val.ival;
        int result = 0;
        int valid = 1;

        if (strcmp(node->name, "+") == 0)
            result = lval + rval;
        else if (strcmp(node->name, "-") == 0)
            result = lval - rval;
        else if (strcmp(node->name, "*") == 0)
            result = lval * rval;
        else if (strcmp(node->name, "/") == 0)
        {
            if (rval == 0)
                valid = 0;
            else
                result = lval / rval;
        }
        else if (strcmp(node->name, "%") == 0)
        {
            if (rval == 0)
                valid = 0;
            else
                result = lval % rval;
        }
        else if (strcmp(node->name, "<>") == 0)
            result = lval < rval;
        else if (strcmp(node->name, "==") == 0)
            result = lval == rval;
        else if (strcmp(node->name, "!=") == 0)
            result = lval != rval;
        else if (strcmp(node->name, ">=") == 0)
            result = lval >= rval;
        else if (strcmp(node->name, "<=") == 0)
            result = lval <= rval;
        else if (strcmp(node->name, ">") == 0)
            result = lval > rval;
        else if (strcmp(node->name, "&&") == 0)
            result = lval && rval;
        else if (strcmp(node->name, "||") == 0)
            result = lval || rval;
        else
            valid = 0;

        if (valid)
        {
            fprintf(stderr,
                    "[Optimizer] Constant folding: %d %s %d = %d\n",
                    lval, node->name, rval, result);
            ast_free(node->left);
            ast_free(node->right);
            node->left = NULL;
            node->right = NULL;
            node->kind = NODE_ICONST;
            node->val.ival = result;
            node->type = TYPE_INT;
            if (node->name)
            {
                free(node->name);
                node->name = NULL;
            }
            return node;
        }
    }

    /* Float constant folding */
    if (node->kind == NODE_BINOP &&
        node->left && node->left->kind == NODE_FCONST &&
        node->right && node->right->kind == NODE_FCONST)
    {

        float lval = node->left->val.fval;
        float rval = node->right->val.fval;
        float result = 0;
        int valid = 1;

        if (strcmp(node->name, "+") == 0)
            result = lval + rval;
        else if (strcmp(node->name, "-") == 0)
            result = lval - rval;
        else if (strcmp(node->name, "*") == 0)
            result = lval * rval;
        else if (strcmp(node->name, "/") == 0)
        {
            if (rval == 0.0f)
                valid = 0;
            else
                result = lval / rval;
        }
        else
            valid = 0;

        if (valid)
        {
            fprintf(stderr,
                    "[Optimizer] Constant folding: %f %s %f = %f\n",
                    lval, node->name, rval, result);
            ast_free(node->left);
            ast_free(node->right);
            node->left = NULL;
            node->right = NULL;
            node->kind = NODE_FCONST;
            node->val.fval = result;
            node->type = TYPE_FLOAT;
            if (node->name)
            {
                free(node->name);
                node->name = NULL;
            }
            return node;
        }
    }

    /* Ειδικές περιπτώσεις */
    if (node->kind == NODE_BINOP && node->left && node->right)
    {
        /* x * 0 = 0 */
        if (strcmp(node->name, "*") == 0 &&
            node->right->kind == NODE_ICONST &&
            node->right->val.ival == 0)
        {
            fprintf(stderr, "[Optimizer] x * 0 = 0\n");
            ast_free(node->left);
            ast_free(node->right);
            node->left = node->right = NULL;
            node->kind = NODE_ICONST;
            node->val.ival = 0;
            node->type = TYPE_INT;
            if (node->name)
            {
                free(node->name);
                node->name = NULL;
            }
            return node;
        }
        /* x * 1 = x */
        if (strcmp(node->name, "*") == 0 &&
            node->right->kind == NODE_ICONST &&
            node->right->val.ival == 1)
        {
            fprintf(stderr, "[Optimizer] x * 1 = x\n");
            ASTNode *left = node->left;
            node->left = NULL;
            ast_free(node->right);
            node->right = NULL;
            if (node->name)
            {
                free(node->name);
                node->name = NULL;
            }
            free(node);
            return left;
        }
        /* x + 0 = x */
        if (strcmp(node->name, "+") == 0 &&
            node->right->kind == NODE_ICONST &&
            node->right->val.ival == 0)
        {
            fprintf(stderr, "[Optimizer] x + 0 = x\n");
            ASTNode *left = node->left;
            node->left = NULL;
            ast_free(node->right);
            node->right = NULL;
            if (node->name)
            {
                free(node->name);
                node->name = NULL;
            }
            free(node);
            return left;
        }
        /* x - 0 = x */
        if (strcmp(node->name, "-") == 0 &&
            node->right->kind == NODE_ICONST &&
            node->right->val.ival == 0)
        {
            fprintf(stderr, "[Optimizer] x - 0 = x\n");
            ASTNode *left = node->left;
            node->left = NULL;
            ast_free(node->right);
            node->right = NULL;
            if (node->name)
            {
                free(node->name);
                node->name = NULL;
            }
            free(node);
            return left;
        }
    }

    /* UNOP με σταθερά */
    if (node->kind == NODE_UNOP &&
        node->left && node->left->kind == NODE_ICONST)
    {
        if (strcmp(node->name, "-") == 0)
        {
            int val = -node->left->val.ival;
            fprintf(stderr, "[Optimizer] Unary minus: -%d = %d\n",
                    node->left->val.ival, val);
            ast_free(node->left);
            node->left = NULL;
            node->kind = NODE_ICONST;
            node->val.ival = val;
            node->type = TYPE_INT;
            if (node->name)
            {
                free(node->name);
                node->name = NULL;
            }
            return node;
        }
        if (strcmp(node->name, "!") == 0)
        {
            int val = !node->left->val.ival;
            fprintf(stderr, "[Optimizer] Logical not: !%d = %d\n",
                    node->left->val.ival, val);
            ast_free(node->left);
            node->left = NULL;
            node->kind = NODE_ICONST;
            node->val.ival = val;
            node->type = TYPE_INT;
            if (node->name)
            {
                free(node->name);
                node->name = NULL;
            }
            return node;
        }
    }

    return node;
}

/* ==================== #3 Απαλοιφή Άχρηστου Κώδικα ==================== */
ASTNode *dead_code_elimination(ASTNode *node)
{
    if (!node)
        return NULL;

    node->left = dead_code_elimination(node->left);
    node->right = dead_code_elimination(node->right);
    node->extra = dead_code_elimination(node->extra);

    /* Απαλοιφή κώδικα μετά από return */
    if (node->kind == NODE_COMPOUND && node->left)
    {
        ASTNode *stmt = node->left;
        while (stmt)
        {
            if (stmt->kind == NODE_RETURN)
            {
                if (stmt->next)
                {
                    fprintf(stderr,
                            "[Optimizer] Dead code after return eliminated\n");
                    ast_free(stmt->next);
                    stmt->next = NULL;
                }
                break;
            }
            stmt = stmt->next;
        }
    }

    /* Απαλοιφή if με σταθερή συνθήκη */
    if (node->kind == NODE_IF && node->left &&
        node->left->kind == NODE_ICONST)
    {
        if (node->left->val.ival != 0)
        {
            fprintf(stderr,
                    "[Optimizer] if(true) eliminated - keeping then branch\n");
            ASTNode *then_br = node->right;
            ASTNode *next = node->next;
            node->right = NULL;
            node->next = NULL;
            ast_free(node);
            if (then_br)
                then_br->next = next;
            return then_br ? then_br : next;
        }
        else
        {
            fprintf(stderr,
                    "[Optimizer] if(false) eliminated - keeping else branch\n");
            ASTNode *else_br = node->extra;
            ASTNode *next = node->next;
            node->extra = NULL;
            node->next = NULL;
            ast_free(node);
            if (else_br)
                else_br->next = next;
            return else_br ? else_br : next;
        }
    }

    /* Απαλοιφή while(false) */
    if (node->kind == NODE_WHILE && node->left &&
        node->left->kind == NODE_ICONST &&
        node->left->val.ival == 0)
    {
        fprintf(stderr, "[Optimizer] while(false) eliminated\n");
        ASTNode *next = node->next;
        node->next = NULL;
        ast_free(node);
        return next;
    }

    return node;
}

/* ==================== Κύρια συνάρτηση βελτιστοποίησης ==================== */
ASTNode *optimize(ASTNode *root)
{
    if (!root)
        return NULL;

    fprintf(stderr, "\n========== Optimization ==========\n");

    /* #4 Διάδοση αντιγράφου πρώτα */
    copy_table_clear();
    root = copy_propagation(root);

    /* #1 Αποτίμηση σταθερών */
    root = constant_folding(root);

    /* #3 Απαλοιφή άχρηστου κώδικα */
    root = dead_code_elimination(root);

    fprintf(stderr, "Optimization complete.\n");
    return root;
}