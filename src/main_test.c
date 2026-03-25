#include <stdio.h>
#include "symtable.h"

extern int yyparse();
extern FILE *yyin;
extern SymTable *symtable;

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
    symtable_destroy(symtable);
    return 0;
}