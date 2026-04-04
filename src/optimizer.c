#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "optimizer.h"

/* ==================== #1 Αποτίμηση Σταθερών ==================== */
ASTNode *constant_folding(ASTNode *node)
{
    if (!node)
        return NULL;

    /* Αναδρομική εφαρμογή σε όλους τους κόμβους */
    node->left = constant_folding(node->left);
    node->right = constant_folding(node->right);
    node->extra = constant_folding(node->extra);

    /* Μόνο για BINOP με δύο σταθερές */
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
            {
                valid = 0;
            }
            else
                result = lval / rval;
        }
        else if (strcmp(node->name, "%") == 0)
        {
            if (rval == 0)
            {
                valid = 0;
            }
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
            fprintf(stderr, "[Optimizer] Constant folding: %d %s %d = %d\n",
                    lval, node->name, rval, result);
            /* Αντικατάσταση με σταθερά */
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
            {
                valid = 0;
            }
            else
                result = lval / rval;
        }
        else
            valid = 0;

        if (valid)
        {
            fprintf(stderr, "[Optimizer] Constant folding: %f %s %f = %f\n",
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
            ast_free(node);
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
            ast_free(node);
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
            ast_free(node);
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

    /* Αναδρομική εφαρμογή */
    node->left = dead_code_elimination(node->left);
    node->right = dead_code_elimination(node->right);
    node->extra = dead_code_elimination(node->extra);

    /* Απαλοιφή κώδικα μετά από return σε compound */
    if (node->kind == NODE_COMPOUND && node->left)
    {
        ASTNode *stmt = node->left;
        ASTNode *prev = NULL;
        while (stmt)
        {
            if (stmt->kind == NODE_RETURN)
            {
                /* Διαγραφή όλων των επόμενων εντολών */
                if (stmt->next)
                {
                    fprintf(stderr,
                            "[Optimizer] Dead code after return eliminated\n");
                    ast_free(stmt->next);
                    stmt->next = NULL;
                }
                break;
            }
            prev = stmt;
            stmt = stmt->next;
        }
    }

    /* Απαλοιφή if με σταθερή συνθήκη */
    if (node->kind == NODE_IF && node->left)
    {
        if (node->left->kind == NODE_ICONST)
        {
            if (node->left->val.ival != 0)
            {
                /* if (true) → κρατάμε μόνο το then */
                fprintf(stderr,
                        "[Optimizer] if(true) eliminated - keeping then branch\n");
                ASTNode *then_br = node->right;
                node->right = NULL;
                ASTNode *next = node->next;
                ast_free(node);
                if (then_br)
                    then_br->next = next;
                return then_br;
            }
            else
            {
                /* if (false) → κρατάμε μόνο το else */
                fprintf(stderr,
                        "[Optimizer] if(false) eliminated - keeping else branch\n");
                ASTNode *else_br = node->extra;
                node->extra = NULL;
                ASTNode *next = node->next;
                ast_free(node);
                if (else_br)
                    else_br->next = next;
                return else_br ? else_br : next;
            }
        }
    }

    /* Απαλοιφή while με σταθερή false συνθήκη */
    if (node->kind == NODE_WHILE && node->left)
    {
        if (node->left->kind == NODE_ICONST &&
            node->left->val.ival == 0)
        {
            fprintf(stderr,
                    "[Optimizer] while(false) eliminated\n");
            ASTNode *next = node->next;
            ast_free(node);
            return next;
        }
    }

    return node;
}

/* ==================== Κύρια συνάρτηση βελτιστοποίησης ==================== */
ASTNode *optimize(ASTNode *root)
{
    if (!root)
        return NULL;

    fprintf(stderr, "\n========== Optimization ==========\n");

    /* Εφαρμογή βελτιστοποιήσεων */
    root = constant_folding(root);
    root = dead_code_elimination(root);

    fprintf(stderr, "Optimization complete.\n");
    return root;
}