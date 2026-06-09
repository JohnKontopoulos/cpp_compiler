/*
 * semantic.c
 * Υλοποίηση Σημασιολογικού Αναλυτή για τον μεταγλωττιστή CPP
 *
 * Εκτελεί σημασιολογικούς ελέγχους στο ΑΣΔ:
 * 1. Έλεγχος τύπων εκφράσεων
 * 2. Έλεγχος δηλώσεων (undeclared identifiers)
 * 3. Αυτόματη εισαγωγή cast nodes (int↔float)
 * 4. Έλεγχος κλήσεων συναρτήσεων
 * 5. Έλεγχος συνθηκών δομών ελέγχου
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "semantic.h"

/* Μετρητής σημασιολογικών σφαλμάτων */
int sem_error_count = 0;

/*
 * sem_error - Εκτύπωση μηνύματος σφάλματος
 * Αυξάνει τον μετρητή σφαλμάτων sem_error_count
 */
void sem_error(const char *msg, const char *name)
{
    fprintf(stderr, "Semantic error: %s '%s'\n",
            msg, name ? name : "");
    sem_error_count++;
}

/*
 * sem_warning - Εκτύπωση προειδοποίησης
 * Δεν αυξάνει τον μετρητή σφαλμάτων
 */
void sem_warning(const char *msg, const char *name)
{
    fprintf(stderr, "Semantic warning: %s '%s'\n",
            msg, name ? name : "");
}

/*
 * types_compatible - Έλεγχος συμβατότητας δύο τύπων
 * Συμβατοί είναι: ίδιοι τύποι ή αριθμητικοί (int/float)
 */
int types_compatible(SymType t1, SymType t2)
{
    if (t1 == t2)
        return 1;
    /* int και float είναι συμβατοί (με αυτόματη μετατροπή) */
    if ((t1 == TYPE_INT || t1 == TYPE_FLOAT) &&
        (t2 == TYPE_INT || t2 == TYPE_FLOAT))
        return 1;
    return 0;
}

/*
 * result_type - Τύπος αποτελέσματος αριθμητικής πράξης
 * Ακολουθεί κανόνες προώθησης τύπων: float > int > char
 */
SymType result_type(SymType t1, SymType t2)
{
    if (t1 == TYPE_FLOAT || t2 == TYPE_FLOAT)
        return TYPE_FLOAT;
    if (t1 == TYPE_INT || t2 == TYPE_INT)
        return TYPE_INT;
    return t1;
}

/*
 * count_args - Μέτρημα ορισμάτων κλήσης συνάρτησης
 *
 * Τα ορίσματα μπορεί να είναι:
 * - Συνδεδεμένα με BINOP "," (π.χ. add(a,b) → BINOP, → a,b)
 * - Συνδεδεμένα με next links (απλή έκφραση)
 * Επιστρέφει: αριθμός ορισμάτων
 */
static int count_args(ASTNode *arg, SymTable *st)
{
    if (!arg)
        return 0;

    /* Αν είναι BINOP "," μετράμε αναδρομικά */
    if (arg->kind == NODE_BINOP && arg->name && strcmp(arg->name, ",") == 0)
    {
        int count = 0;
        ASTNode *tmp = arg;
        /* Διαπέραση αριστερά για όλα τα κόμματα */
        while (tmp && tmp->kind == NODE_BINOP &&
               tmp->name && strcmp(tmp->name, ",") == 0)
        {
            semantic_check_expr(tmp->right, st);
            count++;
            tmp = tmp->left;
        }
        /* Το τελευταίο αριστερό παιδί */
        if (tmp)
        {
            semantic_check_expr(tmp, st);
            count++;
        }
        return count;
    }

    /* Απλή περίπτωση: ένα μόνο όρισμα */
    semantic_check_expr(arg, st);
    return 1;
}

/*
 * semantic_check_expr - Σημασιολογικός έλεγχος έκφρασης
 *
 * Διαπερνά αναδρομικά το ΑΣΔ και:
 * - Ελέγχει ότι τα αναγνωριστικά έχουν δηλωθεί
 * - Ελέγχει συμβατότητα τύπων
 * - Εισάγει cast nodes για αυτόματες μετατροπές
 * - Υπολογίζει και αποθηκεύει τον τύπο κάθε κόμβου
 *
 * Επιστρέφει: τύπος αποτελέσματος της έκφρασης
 */
SymType semantic_check_expr(ASTNode *node, SymTable *st)
{
    if (!node)
        return TYPE_UNKNOWN;

    switch (node->kind)
    {

    /* ===== Σταθερές: ο τύπος είναι γνωστός ===== */
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

    /* ===== Αναγνωριστικό: αναζήτηση στον ΠΣ ===== */
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

    /* ===== Δυαδικός τελεστής ===== */
    case NODE_BINOP:
    {
        SymType lt = semantic_check_expr(node->left, st);
        SymType rt = semantic_check_expr(node->right, st);

        /* Λογικοί τελεστές: απαιτούν int */
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

        /* Τελεστές σύγκρισης: επιστρέφουν int */
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

    /* ===== Μοναδιαίος τελεστής ===== */
    case NODE_UNOP:
    {
        SymType t = semantic_check_expr(node->left, st);
        /* Λογική άρνηση: απαιτεί int */
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

    /* ===== Ανάθεση: έλεγχος και cast ===== */
    case NODE_ASSIGN:
    {
        SymType lt = semantic_check_expr(node->left, st);
        SymType rt = semantic_check_expr(node->right, st);

        if (!types_compatible(lt, rt))
        {
            sem_error("incompatible types in assignment", "=");
        }
        else if (lt != rt)
        {
            /* Εισαγωγή cast node για αυτόματη μετατροπή */
            if (lt == TYPE_FLOAT && rt == TYPE_INT)
            {
                /* int → float */
                ASTNode *cast = ast_make_cast(node->right, TYPE_FLOAT);
                node->right = cast;
                fprintf(stderr, "[Semantic] cast int→float added\n");
            }
            else if (lt == TYPE_INT && rt == TYPE_FLOAT)
            {
                /* float → int: αποκοπή κλασματικού */
                ASTNode *cast = ast_make_cast(node->right, TYPE_INT);
                node->right = cast;
                fprintf(stderr, "[Semantic] cast float→int added\n");
            }
        }
        node->type = lt;
        return lt;
    }

    /* ===== Κλήση συνάρτησης ===== */
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
        {
            sem_error("not a function", node->name);
            node->type = TYPE_UNKNOWN;
            return TYPE_UNKNOWN;
        }

        /*
         * Έλεγχος αριθμού παραμέτρων
         * Τα ορίσματα μπορεί να είναι συνδεδεμένα με BINOP ","
         * ή με next links — χρησιμοποιούμε count_args()
         */
        int actual_count = count_args(node->left, st);

        if (s->param_count > 0 && actual_count != s->param_count)
        {
            fprintf(stderr,
                    "Semantic error: function '%s' expects %d params but got %d\n",
                    node->name, s->param_count, actual_count);
            sem_error_count++;
        }

        node->type = s->type;
        return s->type;
    }

    default:
        return TYPE_UNKNOWN;
    }
}

/*
 * semantic_check_stmt - Σημασιολογικός έλεγχος εντολής
 *
 * Ελέγχει κάθε τύπο εντολής:
 * - Εκφράσεις: καλεί semantic_check_expr
 * - if/while: ελέγχει ότι η συνθήκη είναι int
 * - for: ελέγχει init, cond, step, body
 * - Σύνθετη εντολή: ελέγχει όλες τις εντολές της λίστας
 * - cout/cin: ελέγχει όλες τις εκφράσεις/μεταβλητές
 */
void semantic_check_stmt(ASTNode *node, SymTable *st)
{
    if (!node)
        return;

    switch (node->kind)
    {
    /* Εκφράσεις ως εντολές */
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

    /* if-then-else: η συνθήκη πρέπει να είναι int */
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

    /* while: η συνθήκη πρέπει να είναι int */
    case NODE_WHILE:
    {
        SymType ct = semantic_check_expr(node->left, st);
        if (ct != TYPE_INT && ct != TYPE_UNKNOWN)
            sem_error("while condition must be int", "while");
        semantic_check_stmt(node->right, st);
        break;
    }

    /* for: έλεγχος όλων των μερών */
    case NODE_FOR:
        semantic_check_expr(node->left, st);  /* init */
        semantic_check_expr(node->right, st); /* cond */
        semantic_check_expr(node->extra, st); /* step */
        semantic_check_stmt(node->next, st);  /* body */
        break;

    /* return: έλεγχος τιμής επιστροφής */
    case NODE_RETURN:
        semantic_check_expr(node->left, st);
        break;

    /* Σύνθετη εντολή: έλεγχος όλων των εντολών */
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

    /* cout: έλεγχος εκφράσεων εξόδου */
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

    /* cin: έλεγχος μεταβλητών εισόδου */
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

/*
 * semantic_check_program - Κύριος σημασιολογικός έλεγχος
 *
 * Καλείται μετά το parsing για έλεγχο ολόκληρου του προγράμματος.
 * Εκτυπώνει σύνοψη αποτελεσμάτων.
 */
void semantic_check_program(ASTNode *root, SymTable *st)
{
    if (!root)
        return;
    fprintf(stderr, "\n========== Semantic Analysis ==========\n");
    semantic_check_stmt(root, st);
    if (sem_error_count == 0)
        fprintf(stderr, "No semantic errors found!\n");
    else
        fprintf(stderr, "%d semantic error(s) found.\n", sem_error_count);
}