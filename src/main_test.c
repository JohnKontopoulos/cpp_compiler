#include <stdio.h>
#include "symtable.h"
#include "ast.h"
#include "semantic.h"

extern int yyparse();
extern FILE *yyin;
extern SymTable *symtable;
extern ASTNode *ast_root;

static void print_ast(ASTNode *root)
{
    printf("\n==================== AST ====================\n");
    if (!root)
    {
        printf("(empty)\n");
        return;
    }
    if (root->kind == NODE_COMPOUND)
    {
        printf("[COMPOUND] <unknown>\n");
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
    if (argc > 1)
    {
        yyin = fopen(argv[1], "r");
        if (!yyin)
        {
            fprintf(stderr, "Cannot open file: %s\n", argv[1]);
            return 1;
        }
    }
    symtable = symtable_create();
    yyparse();

    print_ast(ast_root);

    /* Σημασιολογικός έλεγχος */
    semantic_check_program(ast_root, symtable);

    ast_free(ast_root);
    symtable_destroy(symtable);
    return 0;
}