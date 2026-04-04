/*
 * ast.h
 * Αφηρημένο Συντακτικό Δέντρο (ΑΣΔ) για τον μεταγλωττιστή CPP
 *
 * Ορίζει τη δομή των κόμβων του ΑΣΔ και τις συναρτήσεις
 * κατασκευής τους. Κάθε κόμβος αντιστοιχεί σε μια κατασκευή
 * της γλώσσας (έκφραση, εντολή, δήλωση).
 *
 * Δομή κόμβου:
 * - left:  αριστερό παιδί (πρώτος τελεστέος, συνθήκη, κλπ)
 * - right: δεξί παιδί (δεύτερος τελεστέος, then-branch, κλπ)
 * - extra: τρίτο παιδί (else-branch, step σε for, κλπ)
 * - next:  επόμενη εντολή στη λίστα εντολών
 */

#ifndef AST_H
#define AST_H

#include "symtable.h"

/* ==================== ΤΥΠΟΙ ΚΟΜΒΩΝ ==================== */
typedef enum
{
    /* ===== Εκφράσεις ===== */
    NODE_ICONST, /* Ακέραια σταθερά: val.ival */
    NODE_FCONST, /* Πραγματική σταθερά: val.fval */
    NODE_CCONST, /* Σταθερά χαρακτήρα: val.cval */
    NODE_SCONST, /* Συμβολοσειρά: val.sval */
    NODE_ID,     /* Αναγνωριστικό: name */
    NODE_BINOP,  /* Δυαδικός τελεστής: name=op, left, right */
    NODE_UNOP,   /* Μοναδιαίος τελεστής: name=op, left */
    NODE_ASSIGN, /* Ανάθεση: left=lvalue, right=rvalue */
    NODE_CALL,   /* Κλήση συνάρτησης: name, left=args */
    NODE_INDEX,  /* Δεικτοδότηση πίνακα: left[right] */
    NODE_FIELD,  /* Πρόσβαση πεδίου: left.right */
    NODE_INCDEC, /* Αύξηση/μείωση (++/--) */
    NODE_LIST,   /* Έκφραση λίστας */
    NODE_CAST,   /* Μετατροπή τύπου: left→type */

    /* ===== Εντολές ===== */
    NODE_EXPR_STMT, /* Έκφραση ως εντολή */
    NODE_IF,        /* if: left=cond, right=then, extra=else */
    NODE_WHILE,     /* while: left=cond, right=body */
    NODE_FOR,       /* for: left=init, right=cond, extra=step, next=body */
    NODE_RETURN,    /* return: left=expr */
    NODE_BREAK,     /* break */
    NODE_CONTINUE,  /* continue */
    NODE_COMPOUND,  /* Σύνθετη εντολή {}: left=stmts */
    NODE_CIN,       /* cin >>: left=vars */
    NODE_COUT,      /* cout <<: left=exprs */

    /* ===== Δηλώσεις ===== */
    NODE_VAR_DECL,  /* Δήλωση μεταβλητής */
    NODE_FUNC_DECL, /* Δήλωση συνάρτησης */
    NODE_PARAM,     /* Παράμετρος */

    /* ===== Πρόγραμμα ===== */
    NODE_PROGRAM, /* Ρίζα ΑΣΔ */
    NODE_FUNC_DEF /* Ορισμός συνάρτησης */
} NodeKind;

/* ==================== ΚΟΜΒΟΣ ΑΣΔ ==================== */
typedef struct ASTNode
{
    NodeKind kind; /* Τύπος κόμβου */
    SymType type;  /* Τύπος δεδομένων έκφρασης */
    int is_lvalue; /* 1 αν είναι lvalue (μπορεί να ανατεθεί) */

    /* Τιμή σταθεράς (union για οικονομία μνήμης) */
    union
    {
        int ival;   /* Ακέραια τιμή */
        float fval; /* Πραγματική τιμή */
        char cval;  /* Τιμή χαρακτήρα */
        char *sval; /* Συμβολοσειρά */
        char op;    /* Τελεστής (εναλλακτικά) */
    } val;

    char *name; /* Όνομα (ID, τελεστής, συνάρτηση) */

    /* Παιδιά κόμβου */
    struct ASTNode *left;  /* Αριστερό παιδί */
    struct ASTNode *right; /* Δεξί παιδί */
    struct ASTNode *extra; /* Τρίτο παιδί (else, step) */
    struct ASTNode *next;  /* Επόμενη εντολή στη λίστα */
} ASTNode;

/* ==================== ΚΑΤΑΣΚΕΥΗ ΚΟΜΒΩΝ ==================== */
/* Σταθερές */
ASTNode *ast_make_iconst(int val);
ASTNode *ast_make_fconst(float val);
ASTNode *ast_make_cconst(char val);
ASTNode *ast_make_sconst(char *val);

/* Αναγνωριστικό */
ASTNode *ast_make_id(char *name, SymType type);

/* Τελεστές */
ASTNode *ast_make_binop(char *op, ASTNode *left, ASTNode *right);
ASTNode *ast_make_unop(char *op, ASTNode *operand);
ASTNode *ast_make_assign(ASTNode *left, ASTNode *right);

/* Κλήση συνάρτησης */
ASTNode *ast_make_call(char *name, ASTNode *args);

/* Δομές ελέγχου */
ASTNode *ast_make_if(ASTNode *cond, ASTNode *then_br, ASTNode *else_br);
ASTNode *ast_make_while(ASTNode *cond, ASTNode *body);
ASTNode *ast_make_for(ASTNode *init, ASTNode *cond,
                      ASTNode *step, ASTNode *body);
ASTNode *ast_make_return(ASTNode *expr);
ASTNode *ast_make_compound(ASTNode *stmts);

/* Δηλώσεις */
ASTNode *ast_make_var_decl(char *name, SymType type);

/* Είσοδος/Έξοδος */
ASTNode *ast_make_cout(ASTNode *exprs);
ASTNode *ast_make_cin(ASTNode *vars);

/* Βοηθητικές */
ASTNode *ast_make_node_simple(NodeKind kind);
ASTNode *ast_make_cast(ASTNode *expr, SymType to_type);
ASTNode *ast_append(ASTNode *list, ASTNode *node);

/* Εκτύπωση ΑΣΔ στο stderr */
void ast_print(ASTNode *node, int indent);

/* Απελευθέρωση μνήμης */
void ast_free(ASTNode *node);

#endif