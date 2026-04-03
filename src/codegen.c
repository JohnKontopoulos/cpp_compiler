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

/* Βρίσκει offset και storage class μεταβλητής */
static int find_offset(CodeGen *cg, const char *name, int *is_global)
{
    Symbol *s = symtable_lookup(cg->symtable, name);
    if (s)
    {
        *is_global = (s->is_static || s->depth == 0);
        return s->offset;
    }
    *is_global = 0;
    return 0;
}

/* ==================== #4 Φόρτωση R-value από μνήμη ==================== */
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

/* ==================== #4 Αποθήκευση στη μνήμη ==================== */
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

    /* ===== Σταθερές ===== */
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
        codegen_emit(cg, "\tli %s, %d\t# '%c'", reg,
                     (int)node->val.cval, node->val.cval);
        return reg;
    }

    case NODE_SCONST:
    {
        /* Αποθήκευση string στο .data */
        char label[32];
        sprintf(label, "STR%d", cg->label_count++);
        /* Εκτύπωση string constant - θα γίνει στο .data section */
        char *reg = codegen_alloc_reg(cg, TYPE_INT);
        codegen_emit(cg, "\tla %s, %s\t# string", reg, label);
        return reg;
    }

    /* ===== #4 Φόρτωση μεταβλητής (R-value) ===== */
    case NODE_ID:
    {
        int is_global;
        int offset = find_offset(cg, node->name, &is_global);
        char *reg = codegen_alloc_reg(cg, node->type);
        codegen_load(cg, reg, offset, is_global, node->type, node->name);
        return reg;
    }

    /* ===== #4 Cast node ===== */
    case NODE_CAST:
    {
        char *src = codegen_expr(cg, node->left);
        if (node->left->type == TYPE_INT && node->type == TYPE_FLOAT)
        {
            /* int → float */
            char *dst = codegen_alloc_reg(cg, TYPE_FLOAT);
            codegen_emit(cg, "\tmtc1 %s, %s\t# int→float", src, dst);
            codegen_emit(cg, "\tcvt.s.w %s, %s", dst, dst);
            codegen_free_reg(cg, src);
            return dst;
        }
        else if (node->left->type == TYPE_FLOAT && node->type == TYPE_INT)
        {
            /* float → int */
            char *dst = codegen_alloc_reg(cg, TYPE_INT);
            char *tmp = codegen_alloc_reg(cg, TYPE_FLOAT);
            codegen_emit(cg, "\tcvt.w.s %s, %s\t# float→int", tmp, src);
            codegen_emit(cg, "\tmfc1 %s, %s", dst, tmp);
            codegen_free_reg(cg, src);
            codegen_free_reg(cg, tmp);
            return dst;
        }
        return src;
    }

    /* ===== #2 Δυαδικοί τελεστές + αναφορά πίνακα/κλάσης ===== */
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
                codegen_emit(cg, "\tl.s %s, 0(%s)\t# array elem", result, addr);
            else
                codegen_emit(cg, "\tlw %s, 0(%s)\t# array elem", result, addr);

            codegen_free_reg(cg, base);
            codegen_free_reg(cg, index);
            codegen_free_reg(cg, addr);
            return result;
        }

        /* #2 Αναφορά σε πεδίο κλάσης/ένωσης . */
        if (strcmp(node->name, ".") == 0)
        {
            char *base = codegen_expr(cg, node->left);
            char *result = codegen_alloc_reg(cg, node->type);

            int field_offset = 0;
            if (node->right && node->right->name)
            {
                Symbol *s = symtable_lookup(cg->symtable, node->right->name);
                if (s)
                    field_offset = s->offset;
            }

            if (node->type == TYPE_FLOAT)
                codegen_emit(cg, "\tl.s %s, %d(%s)\t# field", result, field_offset, base);
            else
                codegen_emit(cg, "\tlw %s, %d(%s)\t# field", result, field_offset, base);

            codegen_free_reg(cg, base);
            return result;
        }

        /* Υπόλοιποι αριθμητικοί/λογικοί τελεστές */
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

    /* ===== Μοναδιαίοι τελεστές ===== */
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

    /* ===== Ανάθεση ===== */
    case NODE_ASSIGN:
    {
        char *rval = codegen_expr(cg, node->right);

        if (node->left && node->left->kind == NODE_ID)
        {
            int is_global;
            int offset = find_offset(cg, node->left->name, &is_global);

            /* #4 Cast αν χρειάζεται */
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

    /* ===== #3 Κλήση συνάρτησης ===== */
    case NODE_CALL:
    {
        /* #9 Αποθήκευση caller-saved registers ($t0-$t7) */
        codegen_emit(cg, "\t# save caller-saved registers");
        int saved_regs = 0;
        for (int i = 0; i < MAX_REGS; i++)
        {
            if (cg->t_regs[i].in_use)
            {
                codegen_emit(cg, "\taddiu $sp, $sp, -4");
                codegen_emit(cg, "\tsw %s, 0($sp)\t# spill %s",
                             cg->t_regs[i].name, cg->t_regs[i].name);
                saved_regs++;
            }
        }

        /* Αποθήκευση $ra */
        codegen_emit(cg, "\taddiu $sp, $sp, -4");
        codegen_emit(cg, "\tsw $ra, 0($sp)\t# save $ra");

        /* #3α Παράμετροι στη στοίβα */
        int arg_num = 0;
        ASTNode *arg = node->left;
        while (arg)
        {
            char *areg = codegen_expr(cg, arg);
            if (arg_num < 4)
            {
                codegen_emit(cg, "\tmove $a%d, %s\t# arg %d",
                             arg_num, areg, arg_num);
            }
            else
            {
                /* Παράμετροι πέρα των 4 πηγαίνουν στη στοίβα */
                codegen_emit(cg, "\taddiu $sp, $sp, -4");
                codegen_emit(cg, "\tsw %s, 0($sp)\t# arg %d", areg, arg_num);
            }
            codegen_free_reg(cg, areg);
            arg = arg->next;
            arg_num++;
        }

        /* #3β Άλμα με σύνδεση */
        codegen_emit(cg, "\tjal %s\t# call function",
                     node->name ? node->name : "unknown");

        /* Επαναφορά $ra */
        codegen_emit(cg, "\tlw $ra, 0($sp)\t# restore $ra");
        codegen_emit(cg, "\taddiu $sp, $sp, 4");

        /* #9 Επαναφορά caller-saved registers */
        codegen_emit(cg, "\t# restore caller-saved registers");
        for (int i = MAX_REGS - 1; i >= 0; i--)
        {
            if (cg->t_regs[i].in_use)
            {
                codegen_emit(cg, "\tlw %s, 0($sp)\t# restore %s",
                             cg->t_regs[i].name, cg->t_regs[i].name);
                codegen_emit(cg, "\taddiu $sp, $sp, 4");
            }
        }

        /* #3γ Αποτέλεσμα από $v0 */
        char *result = codegen_alloc_reg(cg, node->type);
        if (node->type == TYPE_FLOAT)
            codegen_emit(cg, "\tmov.s %s, $f0\t# return value", result);
        else
            codegen_emit(cg, "\tmove %s, $v0\t# return value", result);
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

        codegen_emit(cg, "%s:\t# while start", lstart);
        char *cond = codegen_expr(cg, node->left);
        codegen_emit(cg, "\tbeq %s, $zero, %s\t# while false", cond, lend);
        codegen_free_reg(cg, cond);

        codegen_stmt(cg, node->right);
        codegen_emit(cg, "\tj %s\t# while loop", lstart);
        codegen_emit(cg, "%s:\t# while end", lend);

        free(lstart);
        free(lend);
        break;
    }

    case NODE_FOR:
    {
        char *lstart = codegen_new_label(cg);
        char *lend = codegen_new_label(cg);

        /* init */
        if (node->left)
        {
            char *r = codegen_expr(cg, node->left);
            codegen_free_reg(cg, r);
        }

        codegen_emit(cg, "%s:\t# for start", lstart);

        /* condition */
        if (node->right)
        {
            char *cond = codegen_expr(cg, node->right);
            codegen_emit(cg, "\tbeq %s, $zero, %s\t# for false", cond, lend);
            codegen_free_reg(cg, cond);
        }

        /* body */
        codegen_stmt(cg, node->next);

        /* step */
        if (node->extra)
        {
            char *r = codegen_expr(cg, node->extra);
            codegen_free_reg(cg, r);
        }

        codegen_emit(cg, "\tj %s\t# for loop", lstart);
        codegen_emit(cg, "%s:\t# for end", lend);

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
                codegen_emit(cg, "\tmov.s $f0, %s\t# return value", reg);
            else
                codegen_emit(cg, "\tmove $v0, %s\t# return value", reg);
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

    /* ===== #5 Έξοδος με _printf ===== */
    case NODE_COUT:
    {
        /* Αποθήκευση $ra */
        codegen_emit(cg, "\taddiu $sp, $sp, -4");
        codegen_emit(cg, "\tsw $ra, 0($sp)");

        ASTNode *item = node->left;
        while (item)
        {
            char *reg = codegen_expr(cg, item);

            if (item->type == TYPE_FLOAT)
            {
                /* _printf με format %f */
                char fmt_label[32];
                sprintf(fmt_label, "FMT%d", cg->label_count++);
                codegen_emit(cg, "\t# cout float");
                codegen_emit(cg, "\tmov.s $f12, %s", reg);
                codegen_emit(cg, "\tli $v0, 2\t# print float syscall");
                codegen_emit(cg, "\tsyscall");
            }
            else if (item->type == TYPE_STRING ||
                     item->kind == NODE_SCONST)
            {
                codegen_emit(cg, "\t# cout string");
                codegen_emit(cg, "\tmove $a0, %s", reg);
                codegen_emit(cg, "\tli $v0, 4\t# print string syscall");
                codegen_emit(cg, "\tsyscall");
            }
            else if (item->type == TYPE_CHAR)
            {
                codegen_emit(cg, "\t# cout char");
                codegen_emit(cg, "\tmove $a0, %s", reg);
                codegen_emit(cg, "\tli $v0, 11\t# print char syscall");
                codegen_emit(cg, "\tsyscall");
            }
            else
            {
                codegen_emit(cg, "\t# cout int");
                codegen_emit(cg, "\tmove $a0, %s", reg);
                codegen_emit(cg, "\tli $v0, 1\t# print int syscall");
                codegen_emit(cg, "\tsyscall");
            }
            codegen_free_reg(cg, reg);
            item = item->next;
        }

        /* Εκτύπωση newline */
        codegen_emit(cg, "\tli $a0, 10\t# newline");
        codegen_emit(cg, "\tli $v0, 11");
        codegen_emit(cg, "\tsyscall");

        /* Επαναφορά $ra */
        codegen_emit(cg, "\tlw $ra, 0($sp)");
        codegen_emit(cg, "\taddiu $sp, $sp, 4");
        break;
    }

    /* ===== #5 Είσοδος με _scanf ===== */
    case NODE_CIN:
    {
        ASTNode *item = node->left;
        while (item)
        {
            int is_global;
            int offset = 0;
            if (item->kind == NODE_ID)
                offset = find_offset(cg, item->name, &is_global);

            if (item->type == TYPE_FLOAT)
            {
                codegen_emit(cg, "\t# cin float");
                codegen_emit(cg, "\tli $v0, 6\t# read float syscall");
                codegen_emit(cg, "\tsyscall");
                if (item->kind == NODE_ID)
                    codegen_store(cg, "$f0", offset, is_global,
                                  TYPE_FLOAT, item->name);
            }
            else if (item->type == TYPE_CHAR)
            {
                codegen_emit(cg, "\t# cin char");
                codegen_emit(cg, "\tli $v0, 12\t# read char syscall");
                codegen_emit(cg, "\tsyscall");
                if (item->kind == NODE_ID)
                    codegen_store(cg, "$v0", offset, is_global,
                                  TYPE_INT, item->name);
            }
            else
            {
                codegen_emit(cg, "\t# cin int");
                codegen_emit(cg, "\tli $v0, 5\t# read int syscall");
                codegen_emit(cg, "\tsyscall");
                if (item->kind == NODE_ID)
                    codegen_store(cg, "$v0", offset, is_global,
                                  TYPE_INT, item->name);
            }
            item = item->next;
        }
        break;
    }

    case NODE_BREAK:
    case NODE_CONTINUE:
        codegen_emit(cg, "\t# break/continue - not fully implemented");
        break;

    default:
        break;
    }
}

/* ==================== #1 Υπολογισμός frame size ==================== */
static int calc_frame_size(CodeGen *cg)
{
    int local_size = 0;
    for (int i = 0; i < MAX_SYMBOLS; i++)
    {
        Symbol *s = cg->symtable->table[i];
        while (s)
        {
            if (s->kind == SYM_VARIABLE && !s->is_static && s->depth > 0)
            {
                int size = dataspace_type_size(s->type);
                local_size += size;
            }
            s = s->next;
        }
    }
    local_size = ((local_size + 7) / 8) * 8;
    return local_size;
}

/* ==================== Παραγωγή κώδικα για το πρόγραμμα ==================== */
void codegen_program(CodeGen *cg, ASTNode *root)
{
    if (!root)
        return;

    /* #1 Τμήμα δεδομένων */
    codegen_emit(cg, "\t.data");

    /* Καθολικές μεταβλητές */
    for (int i = 0; i < cg->dataspace->count; i++)
    {
        DataEntry *e = &cg->dataspace->entries[i];
        if (e->storage == STORAGE_GLOBAL)
        {
            switch (e->type)
            {
            case TYPE_INT:
                codegen_emit(cg, "%s:\t.word 0", e->name);
                break;
            case TYPE_FLOAT:
                codegen_emit(cg, "%s:\t.float 0.0", e->name);
                break;
            case TYPE_CHAR:
                codegen_emit(cg, "%s:\t.byte 0", e->name);
                break;
            case TYPE_STRING:
                codegen_emit(cg, "%s:\t.space 256", e->name);
                break;
            default:
                codegen_emit(cg, "%s:\t.word 0", e->name);
            }
        }
    }

    /* Τμήμα κώδικα */
    codegen_emit(cg, "\t.text");
    codegen_emit(cg, "\t.globl main");
    codegen_emit(cg, "main:");

    /* Ορισμός τρέχουσας συνάρτησης */
    strncpy(cg->current_func, "main", 255);

    /* #1 Υπολογισμός και δέσμευση stack frame */
    int local_size = calc_frame_size(cg);
    int frame_size = local_size + 4;
    frame_size = ((frame_size + 7) / 8) * 8;
    cg->local_size = local_size;

    codegen_emit(cg, "\taddiu $sp, $sp, -%d\t# allocate frame", frame_size);
    codegen_emit(cg, "\tsw $ra, %d($sp)\t# save $ra", frame_size - 4);

    /* Παραγωγή κώδικα για το σώμα */
    if (root->kind == NODE_COMPOUND)
    {
        ASTNode *stmt = root->left;
        while (stmt)
        {
            codegen_stmt(cg, stmt);
            stmt = stmt->next;
        }
    }

    /* Epilogue */
    codegen_emit(cg, "main_exit:");
    codegen_emit(cg, "\tlw $ra, %d($sp)\t# restore $ra", frame_size - 4);
    codegen_emit(cg, "\taddiu $sp, $sp, %d\t# deallocate frame", frame_size);
    codegen_emit(cg, "\tli $v0, 10\t# exit");
    codegen_emit(cg, "\tsyscall");
}