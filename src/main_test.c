#include <stdio.h>
#include "symtable.h"
#include "ast.h"
#include "semantic.h"
#include "dataspace.h"
#include "codegen.h"

extern int yyparse();
extern FILE *yyin;
extern SymTable *symtable;
extern ASTNode *ast_root;
extern DataSpace *dataspace;

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

    yyparse();
    fclose(yyin);

    print_ast(ast_root);
    semantic_check_program(ast_root, symtable);

    /* Παραγωγή κώδικα */
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

    CodeGen *cg = codegen_create(out, symtable, dataspace);
    codegen_program(cg, ast_root);
    codegen_destroy(cg);

    if (out != stdout)
        fclose(out);

    ast_free(ast_root);
    symtable_destroy(symtable);
    dataspace_destroy(dataspace);
    return 0;
}