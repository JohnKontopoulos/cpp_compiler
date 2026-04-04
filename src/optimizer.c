/*
 * optimizer.c
 * Υλοποίηση Βελτιστοποιητή Ενδιάμεσου Κώδικα
 *
 * Εφαρμόζει βελτιστοποιήσεις στο ΑΣΔ πριν την παραγωγή κώδικα.
 * Όλες οι βελτιστοποιήσεις γίνονται in-place στο ΑΣΔ.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "optimizer.h"

/* ==================== #4 ΔΙΑΔΟΣΗ ΑΝΤΙΓΡΑΦΟΥ ==================== */

/* Μέγιστος αριθμός αντιγράφων στον πίνακα */
#define MAX_COPIES 64

/*
 * CopyEntry - Εγγραφή πίνακα αντιγράφων
 * Αποθηκεύει την αντιστοίχιση: var = copy ή var = const_val
 */
typedef struct
{
    char var[64];  /* Μεταβλητή που αντιστοιχίζεται */
    char copy[64]; /* Μεταβλητή-αντίγραφο (αν δεν είναι σταθερά) */
    int is_const;  /* 1 αν η τιμή είναι σταθερά */
    int const_val; /* Τιμή σταθεράς (αν is_const=1) */
} CopyEntry;

/* Στατικός πίνακας αντιγράφων */
static CopyEntry copy_table[MAX_COPIES];
static int copy_count = 0;

/* Μηδενισμός πίνακα αντιγράφων (σε branches) */
static void copy_table_clear()
{
    copy_count = 0;
}

/*
 * copy_table_add - Προσθήκη/ενημέρωση αντιγράφου
 * Αν υπάρχει ήδη εγγραφή για var, ενημερώνεται
 */
static void copy_table_add(const char *var, const char *copy,
                           int is_const, int const_val)
{
    /* Αναζήτηση υπάρχουσας εγγραφής */
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
    /* Νέα εγγραφή */
    strncpy(copy_table[copy_count].var, var, 63);
    strncpy(copy_table[copy_count].copy, copy, 63);
    copy_table[copy_count].is_const = is_const;
    copy_table[copy_count].const_val = const_val;
    copy_count++;
}

/*
 * copy_table_invalidate - Ακύρωση αντιγράφων για μεταβλητή
 * Αφαιρεί όλες τις εγγραφές που αφορούν τη μεταβλητή
 * (είτε ως var είτε ως copy) λόγω νέας ανάθεσης
 */
static void copy_table_invalidate(const char *var)
{
    for (int i = 0; i < copy_count; i++)
    {
        if (strcmp(copy_table[i].var, var) == 0 ||
            strcmp(copy_table[i].copy, var) == 0)
        {
            /* Αντικατάσταση με τελευταία εγγραφή */
            copy_table[i] = copy_table[--copy_count];
            i--;
        }
    }
}

/*
 * copy_table_lookup - Αναζήτηση αντιγράφου για μεταβλητή
 * Επιστρέφει: εγγραφή ή NULL αν δεν βρεθεί
 */
static CopyEntry *copy_table_lookup(const char *var)
{
    for (int i = 0; i < copy_count; i++)
    {
        if (strcmp(copy_table[i].var, var) == 0)
            return &copy_table[i];
    }
    return NULL;
}

/*
 * copy_propagation - Διάδοση αντιγράφου (#4)
 *
 * Διαπερνά το ΑΣΔ και:
 * - Σε ανάθεση x=y ή x=const: αποθηκεύει στον πίνακα αντιγράφων
 * - Σε χρήση μεταβλητής x: αντικαθιστά με αντίγραφο/τιμή αν υπάρχει
 * - Σε branches (if/while/for): ακυρώνει τον πίνακα (control flow)
 */
ASTNode *copy_propagation(ASTNode *node)
{
    if (!node)
        return NULL;

    switch (node->kind)
    {

    /* Ανάθεση: ενημέρωση πίνακα αντιγράφων */
    case NODE_ASSIGN:
    {
        node->right = copy_propagation(node->right);

        if (node->left && node->left->kind == NODE_ID)
        {
            const char *lhs = node->left->name;
            /* Ακύρωση παλιών αντιγράφων για lhs */
            copy_table_invalidate(lhs);

            if (node->right && node->right->kind == NODE_ID)
            {
                /* x = y: αποθήκευση αντιγράφου */
                fprintf(stderr,
                        "[Optimizer] Copy propagation: %s = %s\n",
                        lhs, node->right->name);
                copy_table_add(lhs, node->right->name, 0, 0);
            }
            else if (node->right && node->right->kind == NODE_ICONST)
            {
                /* x = const: αποθήκευση σταθεράς */
                fprintf(stderr,
                        "[Optimizer] Copy propagation: %s = %d\n",
                        lhs, node->right->val.ival);
                copy_table_add(lhs, "", 1, node->right->val.ival);
            }
        }
        return node;
    }

    /* Αναγνωριστικό: αντικατάσταση αν υπάρχει αντίγραφο */
    case NODE_ID:
    {
        CopyEntry *e = copy_table_lookup(node->name);
        if (e)
        {
            if (e->is_const)
            {
                /* Αντικατάσταση με σταθερά */
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
                /* Αντικατάσταση με άλλη μεταβλητή */
                fprintf(stderr,
                        "[Optimizer] Copy propagation: replace %s with %s\n",
                        node->name, e->copy);
                free(node->name);
                node->name = strdup(e->copy);
            }
        }
        return node;
    }

    /* Branches: ακύρωση πίνακα λόγω control flow */
    case NODE_IF:
    case NODE_WHILE:
    case NODE_FOR:
    {
        node->left = copy_propagation(node->left);
        copy_table_clear(); /* Ακύρωση: δεν ξέρουμε ποιο branch εκτελείται */
        node->right = copy_propagation(node->right);
        node->extra = copy_propagation(node->extra);
        copy_table_clear();
        if (node->next)
            node->next = copy_propagation(node->next);
        return node;
    }

    /* Σύνθετη εντολή: διαπέραση λίστας εντολών */
    case NODE_COMPOUND:
    {
        ASTNode *stmt = node->left;
        ASTNode *prev = NULL;
        ASTNode *first = NULL;

        while (stmt)
        {
            ASTNode *next = stmt->next;
            stmt->next = NULL;

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

    /* Υπόλοιποι κόμβοι: αναδρομή */
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

/* ==================== #1 ΑΠΟΤΙΜΗΣΗ ΣΤΑΘΕΡΩΝ ==================== */

/*
 * constant_folding - Αποτίμηση σταθερών εκφράσεων (#1)
 *
 * Αναδρομικά αποτιμά εκφράσεις όπου και τα δύο ορίσματα
 * είναι σταθερές, και αντικαθιστά τον κόμβο με το αποτέλεσμα.
 *
 * Επίσης χειρίζεται ειδικές περιπτώσεις:
 * - x * 0 = 0
 * - x * 1 = x
 * - x + 0 = x
 * - x - 0 = x
 * - Μοναδιαίος μείον: -(const) = -const
 * - Λογική άρνηση: !(const) = !const
 */
ASTNode *constant_folding(ASTNode *node)
{
    if (!node)
        return NULL;

    /* Πρώτα αναδρομή στα παιδιά */
    node->left = constant_folding(node->left);
    node->right = constant_folding(node->right);
    node->extra = constant_folding(node->extra);

    /* ===== Int constant folding ===== */
    if (node->kind == NODE_BINOP &&
        node->left && node->left->kind == NODE_ICONST &&
        node->right && node->right->kind == NODE_ICONST)
    {

        int lval = node->left->val.ival;
        int rval = node->right->val.ival;
        int result = 0, valid = 1;

        if (strcmp(node->name, "+") == 0)
            result = lval + rval;
        else if (strcmp(node->name, "-") == 0)
            result = lval - rval;
        else if (strcmp(node->name, "*") == 0)
            result = lval * rval;
        else if (strcmp(node->name, "/") == 0)
        {
            if (rval == 0)
                valid = 0; /* Αποφυγή διαίρεσης με μηδέν */
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
            fprintf(stderr, "[Optimizer] Constant folding: %d %s %d = %d\n",
                    lval, node->name, rval, result);
            ast_free(node->left);
            ast_free(node->right);
            node->left = node->right = NULL;
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

    /* ===== Float constant folding ===== */
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
            fprintf(stderr, "[Optimizer] Constant folding: %f %s %f = %f\n",
                    lval, node->name, rval, result);
            ast_free(node->left);
            ast_free(node->right);
            node->left = node->right = NULL;
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

    /* ===== Ειδικές περιπτώσεις ===== */
    if (node->kind == NODE_BINOP && node->left && node->right)
    {

        /* x * 0 = 0 */
        if (strcmp(node->name, "*") == 0 &&
            node->right->kind == NODE_ICONST && node->right->val.ival == 0)
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
            node->right->kind == NODE_ICONST && node->right->val.ival == 1)
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
            node->right->kind == NODE_ICONST && node->right->val.ival == 0)
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
            node->right->kind == NODE_ICONST && node->right->val.ival == 0)
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

    /* ===== Μοναδιαίοι τελεστές με σταθερά ===== */
    if (node->kind == NODE_UNOP &&
        node->left && node->left->kind == NODE_ICONST)
    {

        if (strcmp(node->name, "-") == 0)
        {
            /* Αριθμητική άρνηση σταθεράς */
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
            /* Λογική άρνηση σταθεράς */
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

/* ==================== #3 ΑΠΑΛΟΙΦΗ ΑΧΡΗΣΤΟΥ ΚΩΔΙΚΑ ==================== */

/*
 * dead_code_elimination - Απαλοιφή άχρηστου κώδικα (#3)
 *
 * Αφαιρεί:
 * - Κώδικα μετά από return σε compound εντολή
 * - if(true): κρατά μόνο το then branch
 * - if(false): κρατά μόνο το else branch
 * - while(false): αφαιρεί ολόκληρο τον βρόχο
 */
ASTNode *dead_code_elimination(ASTNode *node)
{
    if (!node)
        return NULL;

    /* Αναδρομή στα παιδιά */
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

    /* if(true): αφαίρεση if, κράτηση then branch */
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
            /* if(false): αφαίρεση if, κράτηση else branch */
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

    /* while(false): αφαίρεση ολόκληρου βρόχου */
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

/* ==================== ΚΥΡΙΑ ΣΥΝΑΡΤΗΣΗ ΒΕΛΤΙΣΤΟΠΟΙΗΣΗΣ ==================== */

/*
 * optimize - Κύρια συνάρτηση βελτιστοποίησης
 *
 * Εφαρμόζει τις βελτιστοποιήσεις με τη σειρά:
 * 1. copy_propagation: αντικατάσταση μεταβλητών με τιμές
 * 2. constant_folding: αποτίμηση σταθερών εκφράσεων
 * 3. dead_code_elimination: απαλοιφή άχρηστου κώδικα
 *
 * Η σειρά είναι σημαντική: η copy_propagation δημιουργεί
 * σταθερές που μπορεί να αξιοποιήσει το constant_folding,
 * το οποίο με τη σειρά του δημιουργεί σταθερές συνθήκες
 * που αξιοποιεί το dead_code_elimination.
 */
ASTNode *optimize(ASTNode *root)
{
    if (!root)
        return NULL;

    fprintf(stderr, "\n========== Optimization ==========\n");

    /* #4 Διάδοση αντιγράφου */
    copy_table_clear();
    root = copy_propagation(root);

    /* #1 Αποτίμηση σταθερών */
    root = constant_folding(root);

    /* #3 Απαλοιφή άχρηστου κώδικα */
    root = dead_code_elimination(root);

    fprintf(stderr, "Optimization complete.\n");
    return root;
}