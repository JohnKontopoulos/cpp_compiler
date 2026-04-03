#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "codegen.h"

CodeGen *codegen_create(FILE *out, SymTable *st, DataSpace *ds)
{
    CodeGen *cg = (CodeGen *)calloc(1, sizeof(CodeGen));
    cg->out = out;
    cg->symtable = st;
    cg->dataspace = ds;
    cg->label_count = 0;
    cg->temp_count = 0;
    cg->func_count = 0;
    cg->str_count = 0;

    for (int i = 0; i < MAX_REGS; i++)
    {
        sprintf(cg->t_regs[i].name, "$t%d", i);
        cg->t_regs[i].in_use = 0;
    }
    for (int i = 0; i < MAX_REGS; i++)
    {
        sprintf(cg->f_regs[i].name, "$f%d", i * 2);
        cg->f_regs[i].in_use = 0;
    }
    return cg;
}

void codegen_destroy(CodeGen *cg)
{
    if (cg)
        free(cg);
}

void codegen_emit(CodeGen *cg, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(cg->out, fmt, args);
    va_end(args);
    fprintf(cg->out, "\n");
}

char *codegen_new_label(CodeGen *cg)
{
    char *label = (char *)malloc(32);
    sprintf(label, "L%d", cg->label_count++);
    return label;
}

char *codegen_alloc_reg(CodeGen *cg, SymType type)
{
    if (type == TYPE_FLOAT)
    {
        for (int i = 0; i < MAX_REGS; i++)
        {
            if (!cg->f_regs[i].in_use)
            {
                cg->f_regs[i].in_use = 1;
                return cg->f_regs[i].name;
            }
        }
        /* #8 Διάχυση - spill πρώτο register στη στοίβα */
        fprintf(stderr, "[CodeGen] WARNING: spilling float register %s\n",
                cg->f_regs[0].name);
        codegen_emit(cg, "\taddiu $sp, $sp, -4");
        codegen_emit(cg, "\ts.s %s, 0($sp)\t# spill", cg->f_regs[0].name);
        return cg->f_regs[0].name;
    }
    else
    {
        for (int i = 0; i < MAX_REGS; i++)
        {
            if (!cg->t_regs[i].in_use)
            {
                cg->t_regs[i].in_use = 1;
                return cg->t_regs[i].name;
            }
        }
        /* #8 Διάχυση - spill πρώτο register στη στοίβα */
        fprintf(stderr, "[CodeGen] WARNING: spilling int register %s\n",
                cg->t_regs[0].name);
        codegen_emit(cg, "\taddiu $sp, $sp, -4");
        codegen_emit(cg, "\tsw %s, 0($sp)\t# spill", cg->t_regs[0].name);
        return cg->t_regs[0].name;
    }
}

void codegen_free_reg(CodeGen *cg, const char *reg)
{
    if (!reg)
        return;
    for (int i = 0; i < MAX_REGS; i++)
    {
        if (strcmp(cg->t_regs[i].name, reg) == 0)
            cg->t_regs[i].in_use = 0;
        if (strcmp(cg->f_regs[i].name, reg) == 0)
            cg->f_regs[i].in_use = 0;
    }
}

void codegen_free_all_regs(CodeGen *cg)
{
    for (int i = 0; i < MAX_REGS; i++)
    {
        cg->t_regs[i].in_use = 0;
        cg->f_regs[i].in_use = 0;
    }
}

void codegen_add_function(CodeGen *cg, const char *name,
                          ASTNode *body, int param_count)
{
    if (cg->func_count >= MAX_FUNCTIONS)
        return;
    strncpy(cg->functions[cg->func_count].name, name, 255);
    cg->functions[cg->func_count].body = body;
    cg->functions[cg->func_count].param_count = param_count;
    cg->func_count++;
}

/* Βρίσκει offset και storage class μεταβλητής */
static int find_offset(CodeGen *cg, const char *name, int *is_global)
{
    Symbol *s = symtable_lookup(cg->symtable, name);
    if (s)
    {
        /* Παράμετροι και τοπικές μεταβλητές είναι πάντα στη στοίβα */
        if (s->kind == SYM_PARAMETER)
        {
            *is_global = 0;
            return s->offset;
        }
        *is_global = (s->is_static || s->depth == 0);
        return s->offset;
    }
    *is_global = 0;
    return 0;
}

/* #4 Φόρτωση R-value από μνήμη */
static void codegen_load(CodeGen *cg, char *reg, int offset,
                         int is_global, SymType type, const char *name)
{
    if (type == TYPE_FLOAT)
    {
        if (is_global)
            codegen_emit(cg, "\tl.s %s, %d($gp)\t# %s", reg, offset, name);
        else
            codegen_emit(cg, "\tl.s %s, %d($sp)\t# %s", reg, offset, name);
    }
    else
    {
        if (is_global)
            codegen_emit(cg, "\tlw %s, %d($gp)\t# %s", reg, offset, name);
        else
            codegen_emit(cg, "\tlw %s, %d($sp)\t# %s", reg, offset, name);
    }
}

/* Αποθήκευση στη μνήμη */
static void codegen_store(CodeGen *cg, char *reg, int offset,
                          int is_global, SymType type, const char *name)
{
    if (type == TYPE_FLOAT)
    {
        if (is_global)
            codegen_emit(cg, "\ts.s %s, %d($gp)\t# %s", reg, offset, name);
        else
            codegen_emit(cg, "\ts.s %s, %d($sp)\t# %s", reg, offset, name);
    }
    else
    {
        if (is_global)
            codegen_emit(cg, "\tsw %s, %d($gp)\t# %s", reg, offset, name);
        else
            codegen_emit(cg, "\tsw %s, %d($sp)\t# %s", reg, offset, name);
    }
}

/* ==================== Παραγωγή κώδικα για έκφραση ==================== */
char *codegen_expr(CodeGen *cg, ASTNode *node)
{
    if (!node)
        return "$zero";

    switch (node->kind)
    {

    case NODE_ICONST:
    {
        char *reg = codegen_alloc_reg(cg, TYPE_INT);
        codegen_emit(cg, "\tli %s, %d", reg, node->val.ival);
        return reg;
    }

    case NODE_FCONST:
    {
        char *reg = codegen_alloc_reg(cg, TYPE_FLOAT);
        codegen_emit(cg, "\tli.s %s, %f", reg, node->val.fval);
        return reg;
    }

    case NODE_CCONST:
    {
        char *reg = codegen_alloc_reg(cg, TYPE_INT);
        codegen_emit(cg, "\tli %s, %d\t# '%c'",
                     reg, (int)node->val.cval, node->val.cval);
        return reg;
    }

    case NODE_SCONST:
    {
        /* Αποθήκευση string στο .data */
        char label[64];
        sprintf(label, "STR%d", cg->str_count);
        if (cg->str_count < 256)
        {
            strncpy(cg->str_labels[cg->str_count], label, 63);
            strncpy(cg->str_values[cg->str_count],
                    node->val.sval, 511);
            cg->str_count++;
        }
        char *reg = codegen_alloc_reg(cg, TYPE_INT);
        codegen_emit(cg, "\tla %s, %s\t# string const", reg, label);
        return reg;
    }

    /* #4 Φόρτωση μεταβλητής (R-value) */
    case NODE_ID:
    {
        int is_global;
        int offset = find_offset(cg, node->name, &is_global);
        char *reg = codegen_alloc_reg(cg, node->type);
        codegen_load(cg, reg, offset, is_global, node->type, node->name);
        return reg;
    }

    /* #4 Cast node */
    case NODE_CAST:
    {
        char *src = codegen_expr(cg, node->left);
        if (node->left && node->left->type == TYPE_INT && node->type == TYPE_FLOAT)
        {
            char *dst = codegen_alloc_reg(cg, TYPE_FLOAT);
            codegen_emit(cg, "\tmtc1 %s, %s\t# int→float", src, dst);
            codegen_emit(cg, "\tcvt.s.w %s, %s", dst, dst);
            codegen_free_reg(cg, src);
            return dst;
        }
        else if (node->left && node->left->type == TYPE_FLOAT && node->type == TYPE_INT)
        {
            char *tmp = codegen_alloc_reg(cg, TYPE_FLOAT);
            char *dst = codegen_alloc_reg(cg, TYPE_INT);
            codegen_emit(cg, "\tcvt.w.s %s, %s\t# float→int", tmp, src);
            codegen_emit(cg, "\tmfc1 %s, %s", dst, tmp);
            codegen_free_reg(cg, src);
            codegen_free_reg(cg, tmp);
            return dst;
        }
        return src;
    }

    /* #2 Δυαδικοί τελεστές */
    case NODE_BINOP:
    {

        /* #2 Αναφορά σε στοιχείο πίνακα [] */
        if (strcmp(node->name, "[]") == 0)
        {
            char *base = codegen_expr(cg, node->left);
            char *index = codegen_expr(cg, node->right);
            char *addr = codegen_alloc_reg(cg, TYPE_INT);
            char *result = codegen_alloc_reg(cg, node->type);

            int elem_size = dataspace_type_size(node->type);
            codegen_emit(cg, "\tli %s, %d\t# elem_size", addr, elem_size);
            codegen_emit(cg, "\tmul %s, %s, %s", addr, index, addr);
            codegen_emit(cg, "\tadd %s, %s, %s", addr, base, addr);

            if (node->type == TYPE_FLOAT)
                codegen_emit(cg, "\tl.s %s, 0(%s)\t# array elem",
                             result, addr);
            else
                codegen_emit(cg, "\tlw %s, 0(%s)\t# array elem",
                             result, addr);

            codegen_free_reg(cg, base);
            codegen_free_reg(cg, index);
            codegen_free_reg(cg, addr);
            return result;
        }

        /* #2 Αναφορά σε πεδίο κλάσης . */
        if (strcmp(node->name, ".") == 0)
        {
            char *base = codegen_expr(cg, node->left);
            char *result = codegen_alloc_reg(cg, node->type);

            int field_offset = 0;
            if (node->right && node->right->name)
            {
                Symbol *s = symtable_lookup(cg->symtable,
                                            node->right->name);
                if (s)
                    field_offset = s->offset;
            }

            if (node->type == TYPE_FLOAT)
                codegen_emit(cg, "\tl.s %s, %d(%s)\t# field",
                             result, field_offset, base);
            else
                codegen_emit(cg, "\tlw %s, %d(%s)\t# field",
                             result, field_offset, base);

            codegen_free_reg(cg, base);
            return result;
        }

        /* Αριθμητικοί/λογικοί τελεστές */
        char *left = codegen_expr(cg, node->left);
        char *right = codegen_expr(cg, node->right);
        char *result = codegen_alloc_reg(cg, node->type);

        if (strcmp(node->name, "+") == 0)
        {
            if (node->type == TYPE_FLOAT)
                codegen_emit(cg, "\tadd.s %s, %s, %s", result, left, right);
            else
                codegen_emit(cg, "\tadd %s, %s, %s", result, left, right);
        }
        else if (strcmp(node->name, "-") == 0)
        {
            if (node->type == TYPE_FLOAT)
                codegen_emit(cg, "\tsub.s %s, %s, %s", result, left, right);
            else
                codegen_emit(cg, "\tsub %s, %s, %s", result, left, right);
        }
        else if (strcmp(node->name, "*") == 0)
        {
            if (node->type == TYPE_FLOAT)
                codegen_emit(cg, "\tmul.s %s, %s, %s", result, left, right);
            else
                codegen_emit(cg, "\tmul %s, %s, %s", result, left, right);
        }
        else if (strcmp(node->name, "/") == 0)
        {
            if (node->type == TYPE_FLOAT)
                codegen_emit(cg, "\tdiv.s %s, %s, %s", result, left, right);
            else
            {
                codegen_emit(cg, "\tdiv %s, %s", left, right);
                codegen_emit(cg, "\tmflo %s", result);
            }
        }
        else if (strcmp(node->name, "%") == 0)
        {
            codegen_emit(cg, "\tdiv %s, %s", left, right);
            codegen_emit(cg, "\tmfhi %s", result);
        }
        else if (strcmp(node->name, "<>") == 0)
        {
            codegen_emit(cg, "\tslt %s, %s, %s", result, left, right);
        }
        else if (strcmp(node->name, "==") == 0)
        {
            codegen_emit(cg, "\tseq %s, %s, %s", result, left, right);
        }
        else if (strcmp(node->name, "!=") == 0)
        {
            codegen_emit(cg, "\tsne %s, %s, %s", result, left, right);
        }
        else if (strcmp(node->name, ">=") == 0)
        {
            codegen_emit(cg, "\tsge %s, %s, %s", result, left, right);
        }
        else if (strcmp(node->name, "<=") == 0)
        {
            codegen_emit(cg, "\tsle %s, %s, %s", result, left, right);
        }
        else if (strcmp(node->name, ">") == 0)
        {
            codegen_emit(cg, "\tsgt %s, %s, %s", result, left, right);
        }
        else if (strcmp(node->name, "&&") == 0)
        {
            codegen_emit(cg, "\tand %s, %s, %s", result, left, right);
        }
        else if (strcmp(node->name, "||") == 0)
        {
            codegen_emit(cg, "\tor %s, %s, %s", result, left, right);
        }

        codegen_free_reg(cg, left);
        codegen_free_reg(cg, right);
        return result;
    }

    /* Μοναδιαίοι τελεστές */
    case NODE_UNOP:
    {
        char *operand = codegen_expr(cg, node->left);
        char *result = codegen_alloc_reg(cg, node->type);

        if (strcmp(node->name, "-") == 0)
        {
            if (node->type == TYPE_FLOAT)
                codegen_emit(cg, "\tneg.s %s, %s", result, operand);
            else
                codegen_emit(cg, "\tneg %s, %s", result, operand);
        }
        else if (strcmp(node->name, "!") == 0)
        {
            codegen_emit(cg, "\tseq %s, %s, $zero", result, operand);
        }
        else if (strcmp(node->name, "post++") == 0)
        {
            codegen_emit(cg, "\tmove %s, %s", result, operand);
            codegen_emit(cg, "\taddi %s, %s, 1", operand, operand);
            if (node->left && node->left->kind == NODE_ID)
            {
                int is_global;
                int offset = find_offset(cg, node->left->name, &is_global);
                codegen_store(cg, operand, offset, is_global,
                              node->left->type, node->left->name);
            }
        }
        else if (strcmp(node->name, "post--") == 0)
        {
            codegen_emit(cg, "\tmove %s, %s", result, operand);
            codegen_emit(cg, "\tsubi %s, %s, 1", operand, operand);
            if (node->left && node->left->kind == NODE_ID)
            {
                int is_global;
                int offset = find_offset(cg, node->left->name, &is_global);
                codegen_store(cg, operand, offset, is_global,
                              node->left->type, node->left->name);
            }
        }
        else if (strcmp(node->name, "pre++") == 0)
        {
            codegen_emit(cg, "\taddi %s, %s, 1", operand, operand);
            codegen_emit(cg, "\tmove %s, %s", result, operand);
            if (node->left && node->left->kind == NODE_ID)
            {
                int is_global;
                int offset = find_offset(cg, node->left->name, &is_global);
                codegen_store(cg, operand, offset, is_global,
                              node->left->type, node->left->name);
            }
        }
        else if (strcmp(node->name, "pre--") == 0)
        {
            codegen_emit(cg, "\tsubi %s, %s, 1", operand, operand);
            codegen_emit(cg, "\tmove %s, %s", result, operand);
            if (node->left && node->left->kind == NODE_ID)
            {
                int is_global;
                int offset = find_offset(cg, node->left->name, &is_global);
                codegen_store(cg, operand, offset, is_global,
                              node->left->type, node->left->name);
            }
        }

        codegen_free_reg(cg, operand);
        return result;
    }

    /* Ανάθεση */
    case NODE_ASSIGN:
    {
        char *rval = codegen_expr(cg, node->right);

        if (node->left && node->left->kind == NODE_ID)
        {
            int is_global;
            int offset = find_offset(cg, node->left->name, &is_global);

            if (node->left->type == TYPE_FLOAT &&
                node->right && node->right->type == TYPE_INT)
            {
                char *freg = codegen_alloc_reg(cg, TYPE_FLOAT);
                codegen_emit(cg, "\tmtc1 %s, %s", rval, freg);
                codegen_emit(cg, "\tcvt.s.w %s, %s", freg, freg);
                codegen_free_reg(cg, rval);
                rval = freg;
            }
            codegen_store(cg, rval, offset, is_global,
                          node->left->type, node->left->name);
        }
        /* #2 Ανάθεση σε στοιχείο πίνακα */
        else if (node->left && node->left->kind == NODE_BINOP &&
                 strcmp(node->left->name, "[]") == 0)
        {
            char *base = codegen_expr(cg, node->left->left);
            char *index = codegen_expr(cg, node->left->right);
            char *addr = codegen_alloc_reg(cg, TYPE_INT);

            int elem_size = dataspace_type_size(node->left->type);
            codegen_emit(cg, "\tli %s, %d", addr, elem_size);
            codegen_emit(cg, "\tmul %s, %s, %s", addr, index, addr);
            codegen_emit(cg, "\tadd %s, %s, %s", addr, base, addr);

            if (node->left->type == TYPE_FLOAT)
                codegen_emit(cg, "\ts.s %s, 0(%s)", rval, addr);
            else
                codegen_emit(cg, "\tsw %s, 0(%s)", rval, addr);

            codegen_free_reg(cg, base);
            codegen_free_reg(cg, index);
            codegen_free_reg(cg, addr);
        }
        return rval;
    }

    /* #3 Κλήση συνάρτησης */
    case NODE_CALL:
    {
        /* #9 Αποθήκευση caller-saved registers που χρησιμοποιούνται */
        int saved[MAX_REGS];
        int nsaved = 0;
        for (int i = 0; i < MAX_REGS; i++)
        {
            if (cg->t_regs[i].in_use)
            {
                codegen_emit(cg, "\taddiu $sp, $sp, -4");
                codegen_emit(cg, "\tsw %s, 0($sp)\t# spill %s",
                             cg->t_regs[i].name, cg->t_regs[i].name);
                saved[nsaved++] = i;
            }
        }

        /* Αποθήκευση $ra */
        codegen_emit(cg, "\taddiu $sp, $sp, -4");
        codegen_emit(cg, "\tsw $ra, 0($sp)\t# save $ra");

        /* #3α Παράμετροι */
        int arg_num = 0;
        ASTNode *arg = node->left;
        while (arg)
        {
            char *areg = codegen_expr(cg, arg);
            if (arg_num < 4)
            {
                codegen_emit(cg, "\tmove $a%d, %s\t# param %d",
                             arg_num, areg, arg_num);
            }
            else
            {
                codegen_emit(cg, "\taddiu $sp, $sp, -4");
                codegen_emit(cg, "\tsw %s, 0($sp)\t# param %d",
                             areg, arg_num);
            }
            codegen_free_reg(cg, areg);
            arg = arg->next;
            arg_num++;
        }

        /* #3β Άλμα με σύνδεση */
        codegen_emit(cg, "\tjal %s\t# call",
                     node->name ? node->name : "unknown");

        /* Επαναφορά $ra */
        codegen_emit(cg, "\tlw $ra, 0($sp)\t# restore $ra");
        codegen_emit(cg, "\taddiu $sp, $sp, 4");

        /* #9 Επαναφορά caller-saved registers (αντίστροφα) */
        for (int i = nsaved - 1; i >= 0; i--)
        {
            codegen_emit(cg, "\tlw %s, 0($sp)\t# restore %s",
                         cg->t_regs[saved[i]].name,
                         cg->t_regs[saved[i]].name);
            codegen_emit(cg, "\taddiu $sp, $sp, 4");
        }

        /* #3γ Αποτέλεσμα */
        char *result = codegen_alloc_reg(cg, node->type);
        if (node->type == TYPE_FLOAT)
            codegen_emit(cg, "\tmov.s %s, $f0\t# return val", result);
        else
            codegen_emit(cg, "\tmove %s, $v0\t# return val", result);
        return result;
    }

    default:
        return "$zero";
    }
}

/* ==================== Παραγωγή κώδικα για εντολές ==================== */
void codegen_stmt(CodeGen *cg, ASTNode *node)
{
    if (!node)
        return;

    switch (node->kind)
    {
    case NODE_ASSIGN:
    case NODE_BINOP:
    case NODE_UNOP:
    case NODE_CALL:
    {
        char *reg = codegen_expr(cg, node);
        codegen_free_reg(cg, reg);
        break;
    }

    case NODE_IF:
    {
        char *cond = codegen_expr(cg, node->left);
        char *lelse = codegen_new_label(cg);
        char *lend = codegen_new_label(cg);

        codegen_emit(cg, "\tbeq %s, $zero, %s\t# if false", cond, lelse);
        codegen_free_reg(cg, cond);

        codegen_stmt(cg, node->right);

        if (node->extra)
        {
            codegen_emit(cg, "\tj %s", lend);
            codegen_emit(cg, "%s:", lelse);
            codegen_stmt(cg, node->extra);
            codegen_emit(cg, "%s:", lend);
        }
        else
        {
            codegen_emit(cg, "%s:", lelse);
        }

        free(lelse);
        free(lend);
        break;
    }

    case NODE_WHILE:
    {
        char *lstart = codegen_new_label(cg);
        char *lend = codegen_new_label(cg);

        codegen_emit(cg, "%s:\t# while", lstart);
        char *cond = codegen_expr(cg, node->left);
        codegen_emit(cg, "\tbeq %s, $zero, %s", cond, lend);
        codegen_free_reg(cg, cond);

        codegen_stmt(cg, node->right);
        codegen_emit(cg, "\tj %s", lstart);
        codegen_emit(cg, "%s:", lend);

        free(lstart);
        free(lend);
        break;
    }

    case NODE_FOR:
    {
        char *lstart = codegen_new_label(cg);
        char *lend = codegen_new_label(cg);

        if (node->left)
        {
            char *r = codegen_expr(cg, node->left);
            codegen_free_reg(cg, r);
        }

        codegen_emit(cg, "%s:\t# for", lstart);

        if (node->right)
        {
            char *cond = codegen_expr(cg, node->right);
            codegen_emit(cg, "\tbeq %s, $zero, %s", cond, lend);
            codegen_free_reg(cg, cond);
        }

        codegen_stmt(cg, node->next);

        if (node->extra)
        {
            char *r = codegen_expr(cg, node->extra);
            codegen_free_reg(cg, r);
        }

        codegen_emit(cg, "\tj %s", lstart);
        codegen_emit(cg, "%s:", lend);

        free(lstart);
        free(lend);
        break;
    }

    case NODE_RETURN:
    {
        if (node->left)
        {
            char *reg = codegen_expr(cg, node->left);
            if (node->left->type == TYPE_FLOAT)
                codegen_emit(cg, "\tmov.s $f0, %s\t# return val", reg);
            else
                codegen_emit(cg, "\tmove $v0, %s\t# return val", reg);
            codegen_free_reg(cg, reg);
        }
        codegen_emit(cg, "\tj %s_exit\t# return", cg->current_func);
        break;
    }

    case NODE_COMPOUND:
    {
        ASTNode *stmt = node->left;
        while (stmt)
        {
            codegen_stmt(cg, stmt);
            stmt = stmt->next;
        }
        break;
    }

        /* #5 Έξοδος cout */
    case NODE_COUT:
    {
        /* Δημιουργία format string στο .data */
        char fmt_label[32];
        char fmt_str[256] = "";

        /* Χτίζουμε το format string */
        ASTNode *item = node->left;
        while (item)
        {
            if (item->type == TYPE_FLOAT)
                strcat(fmt_str, "%f");
            else if (item->type == TYPE_STRING || item->kind == NODE_SCONST)
                strcat(fmt_str, "%s");
            else if (item->type == TYPE_CHAR)
                strcat(fmt_str, "%c");
            else
                strcat(fmt_str, "%d");
            item = item->next;
        }
        strcat(fmt_str, "\\n");

        /* Αποθήκευση format string στο .data */
        sprintf(fmt_label, "FMT%d", cg->label_count++);
        if (cg->str_count < 256)
        {
            strncpy(cg->str_labels[cg->str_count], fmt_label, 63);
            snprintf(cg->str_values[cg->str_count], 511, "\"%s\"", fmt_str);
            cg->str_count++;
        }

        /* Αποθήκευση $ra */
        codegen_emit(cg, "\taddiu $sp, $sp, -4");
        codegen_emit(cg, "\tsw $ra, 0($sp)\t# save $ra for _printf");

        /* Φόρτωση format string ως $a0 */
        codegen_emit(cg, "\tla $a0, %s\t# printf format", fmt_label);

        /* Φόρτωση υπολοίπων ορισμάτων σε $a1, $a2, ... */
        int arg_num = 1;
        item = node->left;
        while (item)
        {
            char *reg = codegen_expr(cg, item);
            if (arg_num < 4)
            {
                if (item->type == TYPE_FLOAT)
                    codegen_emit(cg, "\tmov.s $f%d, %s\t# printf arg %d",
                                 arg_num * 2, reg, arg_num);
                else
                    codegen_emit(cg, "\tmove $a%d, %s\t# printf arg %d",
                                 arg_num, reg, arg_num);
            }
            else
            {
                codegen_emit(cg, "\taddiu $sp, $sp, -4");
                codegen_emit(cg, "\tsw %s, 0($sp)\t# printf arg %d",
                             reg, arg_num);
            }
            codegen_free_reg(cg, reg);
            item = item->next;
            arg_num++;
        }

        /* Κλήση _printf */
        codegen_emit(cg, "\tjal _printf\t# call _printf");

        /* Επαναφορά $ra */
        codegen_emit(cg, "\tlw $ra, 0($sp)\t# restore $ra");
        codegen_emit(cg, "\taddiu $sp, $sp, 4");
        break;
    }

    case NODE_CIN:
    {
        /* Δημιουργία format string για scanf */
        char fmt_label[32];
        char fmt_str[256] = "";

        ASTNode *item = node->left;
        while (item)
        {
            if (item->type == TYPE_FLOAT)
                strcat(fmt_str, "%f");
            else if (item->type == TYPE_CHAR)
                strcat(fmt_str, "%c");
            else
                strcat(fmt_str, "%d");
            item = item->next;
        }

        /* Αποθήκευση format string στο .data */
        sprintf(fmt_label, "FMT%d", cg->label_count++);
        if (cg->str_count < 256)
        {
            strncpy(cg->str_labels[cg->str_count], fmt_label, 63);
            snprintf(cg->str_values[cg->str_count], 511, "\"%s\"", fmt_str);
            cg->str_count++;
        }

        /* Αποθήκευση $ra */
        codegen_emit(cg, "\taddiu $sp, $sp, -4");
        codegen_emit(cg, "\tsw $ra, 0($sp)\t# save $ra for _scanf");

        /* Φόρτωση format string ως $a0 */
        codegen_emit(cg, "\tla $a0, %s\t# scanf format", fmt_label);

        /* Φόρτωση διευθύνσεων μεταβλητών σε $a1, $a2, ... */
        int arg_num = 1;
        item = node->left;
        while (item)
        {
            if (item->kind == NODE_ID)
            {
                int is_global;
                int offset = find_offset(cg, item->name, &is_global);
                if (arg_num < 4)
                {
                    if (is_global)
                        codegen_emit(cg, "\taddiu $a%d, $gp, %d\t# addr of %s",
                                     arg_num, offset, item->name);
                    else
                        codegen_emit(cg, "\taddiu $a%d, $sp, %d\t# addr of %s",
                                     arg_num, offset, item->name);
                }
                else
                {
                    char *tmp = codegen_alloc_reg(cg, TYPE_INT);
                    if (is_global)
                        codegen_emit(cg, "\taddiu %s, $gp, %d\t# addr of %s",
                                     tmp, offset, item->name);
                    else
                        codegen_emit(cg, "\taddiu %s, $sp, %d\t# addr of %s",
                                     tmp, offset, item->name);
                    codegen_emit(cg, "\taddiu $sp, $sp, -4");
                    codegen_emit(cg, "\tsw %s, 0($sp)", tmp);
                    codegen_free_reg(cg, tmp);
                }
            }
            item = item->next;
            arg_num++;
        }

        /* Κλήση _scanf */
        codegen_emit(cg, "\tjal _scanf\t# call _scanf");

        /* Επαναφορά $ra */
        codegen_emit(cg, "\tlw $ra, 0($sp)\t# restore $ra");
        codegen_emit(cg, "\taddiu $sp, $sp, 4");
        break;
    }

    case NODE_BREAK:
    case NODE_CONTINUE:
        codegen_emit(cg, "\t# break/continue");
        break;

    default:
        break;
    }
}

/* ==================== #10 Παραγωγή κώδικα συνάρτησης ==================== */
void codegen_function(CodeGen *cg, const char *name,
                      ASTNode *body, int param_count)
{
    if (!body)
        return;

    strncpy(cg->current_func, name, 255);
    codegen_free_all_regs(cg);

    /* Label συνάρτησης */
    codegen_emit(cg, "\n%s:\t# function %s", name, name);

    /* #1 Υπολογισμός frame size */
    Symbol *func_sym = symtable_lookup(cg->symtable, name);
    int local_size = 0;

    /* Τοπικές μεταβλητές + παράμετροι */
    for (int i = 0; i < MAX_SYMBOLS; i++)
    {
        Symbol *s = cg->symtable->table[i];
        while (s)
        {
            if ((s->kind == SYM_VARIABLE || s->kind == SYM_PARAMETER) && !s->is_static && s->depth > 0)
            {
                local_size += dataspace_type_size(s->type);
            }
            s = s->next;
        }
    }
    local_size = ((local_size + 7) / 8) * 8;
    cg->local_size = local_size;

    /* Frame: local + $ra + $fp */
    int frame_size = local_size + 8;
    frame_size = ((frame_size + 7) / 8) * 8;

    /* Prologue */
    codegen_emit(cg, "\t# prologue");
    codegen_emit(cg, "\taddiu $sp, $sp, -%d\t# allocate frame", frame_size);
    codegen_emit(cg, "\tsw $ra, %d($sp)\t# save $ra", frame_size - 4);
    codegen_emit(cg, "\tsw $fp, %d($sp)\t# save $fp", frame_size - 8);
    codegen_emit(cg, "\tmove $fp, $sp\t# frame pointer");

    /* #3α Αντιγραφή παραμέτρων από $a0-$a3 στη στοίβα */
    if (func_sym && param_count > 0)
    {
        codegen_emit(cg, "\t# copy params to stack");
        for (int i = 0; i < param_count && i < 4; i++)
        {
            codegen_emit(cg, "\tsw $a%d, %d($sp)\t# param %d",
                         i, i * 4, i);
        }
    }

    /* Παραγωγή κώδικα σώματος */
    if (body->kind == NODE_COMPOUND)
    {
        ASTNode *stmt = body->left;
        while (stmt)
        {
            codegen_stmt(cg, stmt);
            stmt = stmt->next;
        }
    }
    else
    {
        codegen_stmt(cg, body);
    }

    /* Epilogue */
    codegen_emit(cg, "%s_exit:\t# epilogue", name);
    codegen_emit(cg, "\tlw $ra, %d($sp)\t# restore $ra", frame_size - 4);
    codegen_emit(cg, "\tlw $fp, %d($sp)\t# restore $fp", frame_size - 8);
    codegen_emit(cg, "\taddiu $sp, $sp, %d\t# deallocate frame", frame_size);
    codegen_emit(cg, "\tjr $ra\t# return");

    codegen_free_all_regs(cg);
}

/* ==================== Παραγωγή κώδικα προγράμματος ==================== */
void codegen_program(CodeGen *cg, ASTNode *root)
{
    if (!root)
        return;

    /* Πρώτα παράγουμε τον κώδικα σε buffer */
    /* Χρησιμοποιούμε tmp file */
    FILE *tmp = tmpfile();
    FILE *real_out = cg->out;
    cg->out = tmp;

    /* Παραγωγή κώδικα συναρτήσεων */
    for (int i = 0; i < cg->func_count; i++)
    {
        codegen_function(cg,
                         cg->functions[i].name,
                         cg->functions[i].body,
                         cg->functions[i].param_count);
    }

    /* Παραγωγή κώδικα main */
    codegen_emit(cg, "\nmain:\t# main function");
    strncpy(cg->current_func, "main", 255);
    codegen_free_all_regs(cg);

    int local_size = 0;
    for (int i = 0; i < MAX_SYMBOLS; i++)
    {
        Symbol *s = cg->symtable->table[i];
        while (s)
        {
            if (s->kind == SYM_VARIABLE && !s->is_static && s->depth > 0)
                local_size += dataspace_type_size(s->type);
            s = s->next;
        }
    }
    local_size = ((local_size + 7) / 8) * 8;
    cg->local_size = local_size;

    int frame_size = local_size + 4;
    frame_size = ((frame_size + 7) / 8) * 8;

    codegen_emit(cg, "\taddiu $sp, $sp, -%d\t# allocate frame", frame_size);
    codegen_emit(cg, "\tsw $ra, %d($sp)\t# save $ra", frame_size - 4);

    if (root->kind == NODE_COMPOUND)
    {
        ASTNode *stmt = root->left;
        while (stmt)
        {
            codegen_stmt(cg, stmt);
            stmt = stmt->next;
        }
    }

    codegen_emit(cg, "main_exit:");
    codegen_emit(cg, "\tlw $ra, %d($sp)\t# restore $ra", frame_size - 4);
    codegen_emit(cg, "\taddiu $sp, $sp, %d\t# deallocate frame", frame_size);
    codegen_emit(cg, "\tli $v0, 10\t# exit");
    codegen_emit(cg, "\tsyscall");

    /* Τώρα εκτυπώνουμε .data με ΟΛΑ τα strings/formats */
    cg->out = real_out;
    fprintf(cg->out, "\t.data\n");

    /* Format strings για printf/scanf */
    for (int i = 0; i < cg->str_count; i++)
    {
        fprintf(cg->out, "%s:\t.asciiz %s\n",
                cg->str_labels[i], cg->str_values[i]);
    }

    /* Καθολικές μεταβλητές */
    for (int i = 0; i < cg->dataspace->count; i++)
    {
        DataEntry *e = &cg->dataspace->entries[i];
        if (e->storage == STORAGE_GLOBAL)
        {
            switch (e->type)
            {
            case TYPE_INT:
                fprintf(cg->out, "%s:\t.word 0\n", e->name);
                break;
            case TYPE_FLOAT:
                fprintf(cg->out, "%s:\t.float 0.0\n", e->name);
                break;
            case TYPE_CHAR:
                fprintf(cg->out, "%s:\t.byte 0\n", e->name);
                break;
            case TYPE_STRING:
                fprintf(cg->out, "%s:\t.space 256\n", e->name);
                break;
            default:
                fprintf(cg->out, "%s:\t.word 0\n", e->name);
            }
        }
    }

    /* Τμήμα κώδικα */
    fprintf(cg->out, "\t.text\n");
    fprintf(cg->out, "\t.globl main\n");

    /* Αντιγραφή tmp buffer στην πραγματική έξοδο */
    rewind(tmp);
    int c;
    while ((c = fgetc(tmp)) != EOF)
        fputc(c, cg->out);
    fclose(tmp);
}