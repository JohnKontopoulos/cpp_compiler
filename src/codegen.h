#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"
#include "symtable.h"
#include "dataspace.h"

/* Καταχωρητές MIPS */
#define REG_ZERO "$zero"
#define REG_GP "$gp"
#define REG_SP "$sp"
#define REG_FP "$fp"
#define REG_RA "$ra"
#define REG_V0 "$v0"
#define REG_V1 "$v1"
#define REG_A0 "$a0"
#define REG_A1 "$a1"
#define REG_A2 "$a2"
#define REG_A3 "$a3"

#define MAX_REGS 8
#define MAX_TEMPS 256

/* Περιγραφή καταχωρητή */
typedef struct
{
    char name[16];
    int in_use;
    char var_name[256]; /* ποια μεταβλητή κρατά */
} Register;

/* Γεννήτορας κώδικα */
typedef struct
{
    FILE *out; /* αρχείο εξόδου */
    SymTable *symtable;
    DataSpace *dataspace;

    Register t_regs[MAX_REGS]; /* $t0-$t7 */
    Register f_regs[MAX_REGS]; /* $f0-$f7 για float */

    int label_count; /* για μοναδικές ετικέτες */
    int temp_count;  /* για προσωρινές μεταβλητές */

    char current_func[256]; /* τρέχουσα συνάρτηση */
    int local_size;         /* μέγεθος τοπικού ΧΔ */
} CodeGen;

/* Δημιουργία/καταστροφή */
CodeGen *codegen_create(FILE *out, SymTable *st, DataSpace *ds);
void codegen_destroy(CodeGen *cg);

/* Παραγωγή κώδικα */
void codegen_program(CodeGen *cg, ASTNode *root);
void codegen_function(CodeGen *cg, ASTNode *node, const char *name);
char *codegen_expr(CodeGen *cg, ASTNode *node);
void codegen_stmt(CodeGen *cg, ASTNode *node);

/* Διαχείριση καταχωρητών */
char *codegen_alloc_reg(CodeGen *cg, SymType type);
void codegen_free_reg(CodeGen *cg, const char *reg);

/* Βοηθητικές */
char *codegen_new_label(CodeGen *cg);
void codegen_emit(CodeGen *cg, const char *fmt, ...);

#endif