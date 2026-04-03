#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "ast.h"
#include "symtable.h"
#include "dataspace.h"

/* Καταχωρητές MIPS */
#define MAX_REGS 8
#define MAX_FUNCTIONS 64

/* Περιγραφή καταχωρητή */
typedef struct
{
    char name[16];
    int in_use;
    char var_name[256];
} Register;

/* Αποθήκευση συνάρτησης για παραγωγή κώδικα */
typedef struct
{
    char name[256];
    ASTNode *body;
    int param_count;
} FuncDef;

/* Γεννήτορας κώδικα */
typedef struct
{
    FILE *out;
    SymTable *symtable;
    DataSpace *dataspace;

    Register t_regs[MAX_REGS]; /* $t0-$t7 */
    Register f_regs[MAX_REGS]; /* $f0,$f2,...,$f14 */

    int label_count;
    int temp_count;

    char current_func[256];
    int local_size;

    /* Αποθήκευση συναρτήσεων */
    FuncDef functions[MAX_FUNCTIONS];
    int func_count;

    /* String constants για .data section */
    char str_labels[256][64];
    char str_values[256][512];
    int str_count;
} CodeGen;

/* Δημιουργία/καταστροφή */
CodeGen *codegen_create(FILE *out, SymTable *st, DataSpace *ds);
void codegen_destroy(CodeGen *cg);

/* Παραγωγή κώδικα */
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