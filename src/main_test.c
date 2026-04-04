#include <stdio.h>
#include "symtable.h"
#include "ast.h"
#include "semantic.h"
#include "dataspace.h"
#include "codegen.h"
#include "optimizer.h"

extern int yyparse();
extern FILE *yyin;
extern SymTable *symtable;
extern DataSpace *dataspace;
extern ASTNode *ast_root;
extern int error_count;

CodeGen *global_cg = NULL;

static void print_ast(ASTNode *root)
{
    fprintf(stderr, "\n==================== AST ====================\n");
    if (!root)
    {
        fprintf(stderr, "(empty)\n");
        return;
    }
    if (root->kind == NODE_COMPOUND)
    {
        fprintf(stderr, "[COMPOUND] <unknown>\n");
        ASTNode *stmt = root->left;
        while (stmt)
        {
            ast_print(stmt, 1);
            stmt = stmt->next;
        }
    }
    else
    {
        ast_print(root, 0);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <input.cpp> [output.asm]\n", argv[0]);
        return 1;
    }

    yyin = fopen(argv[1], "r");
    if (!yyin)
    {
        fprintf(stderr, "Cannot open file: %s\n", argv[1]);
        return 1;
    }

    symtable = symtable_create();
    dataspace = dataspace_create();

    /* Αρχείο εξόδου */
    FILE *out = stdout;
    if (argc >= 3)
    {
        out = fopen(argv[2], "w");
        if (!out)
        {
            fprintf(stderr, "Cannot open output file: %s\n", argv[2]);
            out = stdout;
        }
    }

    /* Δημιουργία codegen ΠΡΙΝ το parsing */
    global_cg = codegen_create(out, symtable, dataspace);

    /* Parsing */
    yyparse();
    fclose(yyin);

    /* Εκτύπωση ΑΣΔ */
    print_ast(ast_root);

    /* Έλεγχος συντακτικών σφαλμάτων */
    if (error_count > 0)
    {
        fprintf(stderr, "Aborting: %d syntax error(s) found.\n", error_count);
        ast_free(ast_root);
        symtable_destroy(symtable);
        dataspace_destroy(dataspace);
        codegen_destroy(global_cg);
        if (out != stdout)
            fclose(out);
        return 1;
    }

    /* Σημασιολογικός έλεγχος */
    semantic_check_program(ast_root, symtable);

    /* Έλεγχος σημασιολογικών σφαλμάτων */
    if (sem_error_count > 0)
    {
        fprintf(stderr, "Aborting: %d semantic error(s) found.\n", sem_error_count);
        ast_free(ast_root);
        symtable_destroy(symtable);
        dataspace_destroy(dataspace);
        codegen_destroy(global_cg);
        if (out != stdout)
            fclose(out);
        return 1;
    }

    /* Βελτιστοποίηση */
    ast_root = optimize(ast_root);

    /* Παραγωγή κώδικα */
    codegen_program(global_cg, ast_root);
    codegen_destroy(global_cg);

    if (out != stdout)
        fclose(out);

    ast_free(ast_root);
    symtable_destroy(symtable);
    dataspace_destroy(dataspace);
    return 0;
}