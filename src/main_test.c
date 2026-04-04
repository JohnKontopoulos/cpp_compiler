/*
 * main_test.c
 * Κύριο πρόγραμμα μεταγλωττιστή CPP
 *
 * Ορχηστρώνει τα στάδια μεταγλώττισης:
 * 1. Λεκτική + Συντακτική Ανάλυση (yyparse)
 * 2. Εκτύπωση ΑΣΔ
 * 3. Σημασιολογική Ανάλυση
 * 4. Βελτιστοποίηση
 * 5. Παραγωγή MIPS Κώδικα
 *
 * Χρήση: ./compiler <input.cpp> [output.asm]
 * - Αν δεν δοθεί output.asm, ο κώδικας εκτυπώνεται στο stdout
 * - Τα debug/info μηνύματα εκτυπώνονται στο stderr
 */

#include <stdio.h>
#include "symtable.h"
#include "ast.h"
#include "semantic.h"
#include "dataspace.h"
#include "codegen.h"
#include "optimizer.h"

/* Εξωτερικές δηλώσεις από τον parser/lexer */
extern int yyparse();
extern FILE *yyin;
extern SymTable *symtable;
extern DataSpace *dataspace;
extern ASTNode *ast_root;
extern int error_count;

/* Global codegen: δημιουργείται πριν το parsing για αποθήκευση συναρτήσεων */
CodeGen *global_cg = NULL;

/*
 * print_ast - Εκτύπωση Αφηρημένου Συντακτικού Δέντρου
 * Εκτυπώνει το ΑΣΔ στο stderr με κατάλληλη μορφοποίηση
 */
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

/*
 * main - Κύρια συνάρτηση μεταγλωττιστή
 *
 * Ροή εκτέλεσης:
 * 1. Άνοιγμα αρχείου εισόδου
 * 2. Δημιουργία δομών δεδομένων (ΠΣ, ΧΔ, ΓΤΚ)
 * 3. Parsing (lexer + parser + ΑΣΔ + ΠΣ)
 * 4. Εκτύπωση ΑΣΔ
 * 5. Έλεγχος συντακτικών σφαλμάτων
 * 6. Σημασιολογικός έλεγχος
 * 7. Έλεγχος σημασιολογικών σφαλμάτων
 * 8. Βελτιστοποίηση ΑΣΔ
 * 9. Παραγωγή MIPS κώδικα
 */
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <input.cpp> [output.asm]\n", argv[0]);
        return 1;
    }

    /* Άνοιγμα αρχείου εισόδου */
    yyin = fopen(argv[1], "r");
    if (!yyin)
    {
        fprintf(stderr, "Cannot open file: %s\n", argv[1]);
        return 1;
    }

    /* Δημιουργία δομών δεδομένων */
    symtable = symtable_create();
    dataspace = dataspace_create();

    /* Καθορισμός αρχείου εξόδου */
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

    /* Δημιουργία ΓΤΚ ΠΡΙΝ το parsing
     * (χρειάζεται για αποθήκευση συναρτήσεων κατά την ανάλυση) */
    global_cg = codegen_create(out, symtable, dataspace);

    /* Στάδιο 1+2: Parsing */
    yyparse();
    fclose(yyin);

    /* Εκτύπωση ΑΣΔ */
    print_ast(ast_root);

    /* Έλεγχος συντακτικών σφαλμάτων - σταμάτημα αν υπάρχουν */
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

    /* Στάδιο 2: Σημασιολογική Ανάλυση */
    semantic_check_program(ast_root, symtable);

    /* Έλεγχος σημασιολογικών σφαλμάτων - σταμάτημα αν υπάρχουν */
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

    /* Προαιρετικό: Βελτιστοποίηση ΑΣΔ */
    ast_root = optimize(ast_root);

    /* Στάδιο 3: Παραγωγή MIPS κώδικα */
    codegen_program(global_cg, ast_root);
    codegen_destroy(global_cg);

    if (out != stdout)
        fclose(out);

    /* Απελευθέρωση μνήμης */
    ast_free(ast_root);
    symtable_destroy(symtable);
    dataspace_destroy(dataspace);
    return 0;
}