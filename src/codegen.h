/*
 * codegen.h
 * Γεννήτορας Τελικού Κώδικα (ΓΤΚ) για τον μεταγλωττιστή CPP
 *
 * Παράγει κώδικα συμβολικής γλώσσας MIPS από το ΑΣΔ.
 *
 * Αρχιτεκτονική MIPS που χρησιμοποιείται:
 * - $t0-$t7: προσωρινοί καταχωρητές ακεραίων (caller-saved)
 * - $f0,$f2,...: προσωρινοί καταχωρητές float
 * - $a0-$a3: καταχωρητές παραμέτρων
 * - $v0: καταχωρητής επιστροφής
 * - $sp: δείκτης στοίβας (τοπικές μεταβλητές)
 * - $gp: δείκτης καθολικών δεδομένων
 * - $ra: διεύθυνση επιστροφής
 * - $fp: frame pointer
 *
 * Είσοδος/Έξοδος: μέσω _printf/_scanf με format strings
 */

#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"
#include "symtable.h"
#include "dataspace.h"

/* ==================== ΣΤΑΘΕΡΕΣ ==================== */
#define MAX_REGS 8       /* $t0-$t7 και $f0,$f2,...,$f14 */
#define MAX_FUNCTIONS 64 /* Μέγιστος αριθμός συναρτήσεων */

/* ==================== ΚΑΤΑΧΩΡΗΤΕΣ ==================== */
/* Περιγραφή κατάστασης καταχωρητή */
typedef struct
{
    char name[16];      /* Όνομα καταχωρητή (π.χ. "$t0") */
    int in_use;         /* 1 αν χρησιμοποιείται, 0 αν ελεύθερος */
    char var_name[256]; /* Μεταβλητή που περιέχει (για debugging) */
} Register;

/* ==================== ΣΥΝΑΡΤΗΣΕΙΣ ==================== */
/* Αποθήκευση ορισμού συνάρτησης για παραγωγή κώδικα */
typedef struct
{
    char name[256];  /* Όνομα συνάρτησης */
    ASTNode *body;   /* Σώμα συνάρτησης (ΑΣΔ) */
    int param_count; /* Αριθμός παραμέτρων */
} FuncDef;

/* ==================== ΓΕΝΝΗΤΟΡΑΣ ΚΩΔΙΚΑ ==================== */
typedef struct
{
    FILE *out;            /* Αρχείο εξόδου για MIPS κώδικα */
    SymTable *symtable;   /* Πίνακας Συμβόλων */
    DataSpace *dataspace; /* Χώρος Δεδομένων */

    /* Καταχωρητές ακεραίων $t0-$t7 */
    Register t_regs[MAX_REGS];
    /* Καταχωρητές float $f0,$f2,...,$f14 */
    Register f_regs[MAX_REGS];

    int label_count; /* Μετρητής για παραγωγή μοναδικών labels */
    int temp_count;  /* Μετρητής προσωρινών μεταβλητών */

    char current_func[256]; /* Όνομα τρέχουσας συνάρτησης */
    int local_size;         /* Μέγεθος τοπικού χώρου δεδομένων */

    /* Συναρτήσεις προς παραγωγή κώδικα */
    FuncDef functions[MAX_FUNCTIONS];
    int func_count;

    /* String/format constants για .data section */
    char str_labels[256][64];  /* Labels string constants */
    char str_values[256][512]; /* Τιμές string constants */
    int str_count;             /* Πλήθος string constants */
} CodeGen;

/* ==================== ΔΙΕΠΑΦΗ ==================== */
/* Δημιουργία/καταστροφή */
CodeGen *codegen_create(FILE *out, SymTable *st, DataSpace *ds);
void codegen_destroy(CodeGen *cg);

/* Κύριες συναρτήσεις παραγωγής κώδικα */
void codegen_program(CodeGen *cg, ASTNode *root);
void codegen_function(CodeGen *cg, const char *name,
                      ASTNode *body, int param_count);
char *codegen_expr(CodeGen *cg, ASTNode *node);
void codegen_stmt(CodeGen *cg, ASTNode *node);

/* Διαχείριση καταχωρητών */
char *codegen_alloc_reg(CodeGen *cg, SymType type);
void codegen_free_reg(CodeGen *cg, const char *reg);
void codegen_free_all_regs(CodeGen *cg);

/* Βοηθητικές */
char *codegen_new_label(CodeGen *cg);
void codegen_emit(CodeGen *cg, const char *fmt, ...);
void codegen_add_function(CodeGen *cg, const char *name,
                          ASTNode *body, int param_count);

#endif