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

/* Παραγωγή κώδικα για έκφραση */
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
        /* Αποθήκευση float σταθεράς στο .data */
        char label[32];
        sprintf(label, "FC%d", cg->label_count++);
        /* Εκτύπωση στο .data section */
        fprintf(cg->out, "\t# float const %f\n", node->val.fval);
        codegen_emit(cg, "\tli.s %s, %f", reg, node->val.fval);
        return reg;
    }

    case NODE_CCONST:
    {
        char *reg = codegen_alloc_reg(cg, TYPE_INT);
        codegen_emit(cg, "\tli %s, %d\t# '%c'", reg, (int)node->val.cval, node->val.cval);
        return reg;
    }

    case NODE_SCONST:
    {
        char *reg = codegen_alloc_reg(cg, TYPE_INT);
        codegen_emit(cg, "\t# string const: %s", node->val.sval);
        return reg;
    }

    case NODE_ID:
    {
        int is_global;
        int offset = find_offset(cg, node->name, &is_global);
        char *reg = codegen_alloc_reg(cg, node->type);

        if (node->type == TYPE_FLOAT)
        {
            if (is_global)
                codegen_emit(cg, "\tl.s %s, %d($gp)\t# %s", reg, offset, node->name);
            else
                codegen_emit(cg, "\tl.s %s, %d($sp)\t# %s", reg, offset, node->name);
        }
        else
        {
            if (is_global)
                codegen_emit(cg, "\tlw %s, %d($gp)\t# %s", reg, offset, node->name);
            else
                codegen_emit(cg, "\tlw %s, %d($sp)\t# %s", reg, offset, node->name);
        }
        return reg;
    }

    case NODE_BINOP:
    {
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
            {
                codegen_emit(cg, "\tmul.s %s, %s, %s", result, left, right);
            }
            else
            {
                codegen_emit(cg, "\tmul %s, %s, %s", result, left, right);
            }
        }
        else if (strcmp(node->name, "/") == 0)
        {
            if (node->type == TYPE_FLOAT)
            {
                codegen_emit(cg, "\tdiv.s %s, %s, %s", result, left, right);
            }
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
            /* σύγκριση < */
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
                if (is_global)
                    codegen_emit(cg, "\tsw %s, %d($gp)\t# %s", operand, offset, node->left->name);
                else
                    codegen_emit(cg, "\tsw %s, %d($sp)\t# %s", operand, offset, node->left->name);
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
                if (is_global)
                    codegen_emit(cg, "\tsw %s, %d($gp)\t# %s", operand, offset, node->left->name);
                else
                    codegen_emit(cg, "\tsw %s, %d($sp)\t# %s", operand, offset, node->left->name);
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
                if (is_global)
                    codegen_emit(cg, "\tsw %s, %d($gp)\t# %s", operand, offset, node->left->name);
                else
                    codegen_emit(cg, "\tsw %s, %d($sp)\t# %s", operand, offset, node->left->name);
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
                if (is_global)
                    codegen_emit(cg, "\tsw %s, %d($gp)\t# %s", operand, offset, node->left->name);
                else
                    codegen_emit(cg, "\tsw %s, %d($sp)\t# %s", operand, offset, node->left->name);
            }
        }

        codegen_free_reg(cg, operand);
        return result;
    }

    case NODE_ASSIGN:
    {
        char *rval = codegen_expr(cg, node->right);
        if (node->left && node->left->kind == NODE_ID)
        {
            int is_global;
            int offset = find_offset(cg, node->left->name, &is_global);

            /* Αν το δεξί είναι int αλλά το αριστερό float, μετατροπή */
            if (node->left->type == TYPE_FLOAT && node->right->type == TYPE_INT)
            {
                char *freg = codegen_alloc_reg(cg, TYPE_FLOAT);
                codegen_emit(cg, "\tmtc1 %s, %s", rval, freg);
                codegen_emit(cg, "\tcvt.s.w %s, %s", freg, freg);
                codegen_free_reg(cg, rval);
                rval = freg;
            }

            if (node->left->type == TYPE_FLOAT)
            {
                if (is_global)
                    codegen_emit(cg, "\ts.s %s, %d($gp)\t# %s", rval, offset, node->left->name);
                else
                    codegen_emit(cg, "\ts.s %s, %d($sp)\t# %s", rval, offset, node->left->name);
            }
            else
            {
                if (is_global)
                    codegen_emit(cg, "\tsw %s, %d($gp)\t# %s", rval, offset, node->left->name);
                else
                    codegen_emit(cg, "\tsw %s, %d($sp)\t# %s", rval, offset, node->left->name);
            }
        }
        return rval;
    }

    case NODE_CALL:
    {
        /* Αποθήκευση $ra */
        codegen_emit(cg, "\taddiu $sp, $sp, -4");
        codegen_emit(cg, "\tsw $ra, 0($sp)");

        /* Παράμετροι */
        int arg_num = 0;
        ASTNode *arg = node->left;
        while (arg && arg_num < 4)
        {
            char *areg = codegen_expr(cg, arg);
            codegen_emit(cg, "\tmove $a%d, %s", arg_num, areg);
            codegen_free_reg(cg, areg);
            arg = arg->next;
            arg_num++;
        }

        codegen_emit(cg, "\tjal %s", node->name ? node->name : "unknown");

        codegen_emit(cg, "\tlw $ra, 0($sp)");
        codegen_emit(cg, "\taddiu $sp, $sp, 4");

        char *result = codegen_alloc_reg(cg, node->type);
        if (node->type == TYPE_FLOAT)
            codegen_emit(cg, "\tmov.s %s, $f0", result);
        else
            codegen_emit(cg, "\tmove %s, $v0", result);
        return result;
    }

    default:
        return "$zero";
    }
}

/* Παραγωγή κώδικα για εντολές */
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

        codegen_emit(cg, "\tbeq %s, $zero, %s", cond, lelse);
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

        codegen_emit(cg, "%s:", lstart);
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

        codegen_emit(cg, "%s:", lstart);

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
                codegen_emit(cg, "\tmov.s $f0, %s", reg);
            else
                codegen_emit(cg, "\tmove $v0, %s", reg);
            codegen_free_reg(cg, reg);
        }
        codegen_emit(cg, "\tj main_exit");
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

    case NODE_COUT:
    {
        ASTNode *item = node->left;
        while (item)
        {
            char *reg = codegen_expr(cg, item);
            if (item->type == TYPE_FLOAT)
            {
                codegen_emit(cg, "\tmov.s $f12, %s", reg);
                codegen_emit(cg, "\tli $v0, 2");
                codegen_emit(cg, "\tsyscall");
            }
            else if (item->type == TYPE_STRING || item->kind == NODE_SCONST)
            {
                codegen_emit(cg, "\tmove $a0, %s", reg);
                codegen_emit(cg, "\tli $v0, 4");
                codegen_emit(cg, "\tsyscall");
            }
            else
            {
                codegen_emit(cg, "\tmove $a0, %s", reg);
                codegen_emit(cg, "\tli $v0, 1");
                codegen_emit(cg, "\tsyscall");
            }
            codegen_free_reg(cg, reg);
            item = item->next;
        }
        break;
    }

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
                codegen_emit(cg, "\tli $v0, 6");
                codegen_emit(cg, "\tsyscall");
                if (item->kind == NODE_ID)
                {
                    if (is_global)
                        codegen_emit(cg, "\ts.s $f0, %d($gp)", offset);
                    else
                        codegen_emit(cg, "\ts.s $f0, %d($sp)", offset);
                }
            }
            else
            {
                codegen_emit(cg, "\tli $v0, 5");
                codegen_emit(cg, "\tsyscall");
                if (item->kind == NODE_ID)
                {
                    if (is_global)
                        codegen_emit(cg, "\tsw $v0, %d($gp)", offset);
                    else
                        codegen_emit(cg, "\tsw $v0, %d($sp)", offset);
                }
            }
            item = item->next;
        }
        break;
    }

    default:
        break;
    }
}

/* Παραγωγή κώδικα για ολόκληρο το πρόγραμμα */
void codegen_program(CodeGen *cg, ASTNode *root)
{
    if (!root)
        return;

    /* Σταθερό frame size βάσει συμβόλων */
    int local_size = 0;
    /* Ψάχνουμε στον ΠΣ για τοπικές μεταβλητές */
    for (int i = 0; i < MAX_SYMBOLS; i++)
    {
        Symbol *s = cg->symtable->table[i];
        while (s)
        {
            if (s->kind == SYM_VARIABLE && !s->is_static && s->depth > 0)
            {
                int size = 4; /* default */
                if (s->type == TYPE_CHAR)
                    size = 1;
                else if (s->type == TYPE_FLOAT)
                    size = 4;
                else if (s->type == TYPE_STRING)
                    size = 256;
                local_size += size;
            }
            s = s->next;
        }
    }
    /* Alignment σε πολλαπλάσιο του 8 */
    local_size = ((local_size + 7) / 8) * 8;
    cg->local_size = local_size;

    int frame_size = local_size + 4; /* +4 για $ra */
    frame_size = ((frame_size + 7) / 8) * 8;

    codegen_emit(cg, "\t.data");
    codegen_emit(cg, "\t.text");
    codegen_emit(cg, "\t.globl main");
    codegen_emit(cg, "main:");
    codegen_emit(cg, "\taddiu $sp, $sp, -%d\t# stack frame", frame_size);
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

    /* Epilogue - μόνο αν δεν έχει ήδη return */
    codegen_emit(cg, "main_exit:");
    codegen_emit(cg, "\tlw $ra, %d($sp)\t# restore $ra", frame_size - 4);
    codegen_emit(cg, "\taddiu $sp, $sp, %d", frame_size);
    codegen_emit(cg, "\tli $v0, 10");
    codegen_emit(cg, "\tsyscall");
}