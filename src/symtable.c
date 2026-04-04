/*
 * symtable.c
 * Υλοποίηση Πίνακα Συμβόλων για τον μεταγλωττιστή CPP
 *
 * Χρησιμοποιεί hash table με chaining για αποδοτική
 * εισαγωγή και αναζήτηση συμβόλων.
 *
 * Η αναζήτηση επιστρέφει το σύμβολο με το μεγαλύτερο
 * βάθος εμβέλειας (πιο κοντινή δήλωση).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"

/*
 * hash - Υπολογισμός hash value για όνομα συμβόλου
 * Χρησιμοποιεί πολλαπλασιασμό με 31 (κλασική hash function)
 */
static unsigned int hash(const char *name)
{
    unsigned int h = 0;
    while (*name)
        h = h * 31 + (unsigned char)*name++;
    return h % MAX_SYMBOLS;
}

/*
 * symtable_create - Δημιουργία νέου πίνακα συμβόλων
 * Αρχικοποιεί κενό hash table σε βάθος εμβέλειας 0 (καθολική)
 */
SymTable *symtable_create()
{
    SymTable *st = (SymTable *)malloc(sizeof(SymTable));
    memset(st->table, 0, sizeof(st->table));
    st->current_depth = 0;
    return st;
}

/*
 * symtable_destroy - Απελευθέρωση μνήμης πίνακα συμβόλων
 * Απελευθερώνει όλες τις εγγραφές και τον ίδιο τον ΠΣ
 */
void symtable_destroy(SymTable *st)
{
    for (int i = 0; i < MAX_SYMBOLS; i++)
    {
        Symbol *s = st->table[i];
        while (s)
        {
            Symbol *next = s->next;
            free(s);
            s = next;
        }
    }
    free(st);
}

/*
 * symtable_enter_scope - Είσοδος σε νέα εμβέλεια
 * Αυξάνει το βάθος εμβέλειας κατά 1
 */
void symtable_enter_scope(SymTable *st)
{
    st->current_depth++;
    fprintf(stderr, "[Scope] Entering depth %d\n", st->current_depth);
}

/*
 * symtable_exit_scope - Έξοδος από τρέχουσα εμβέλεια
 * Εκτυπώνει τα σύμβολα της εμβέλειας και μειώνει το βάθος
 * Σημείωση: ΔΕΝ διαγράφει τα σύμβολα (χρειάζονται για σημασιολογικό έλεγχο)
 */
void symtable_exit_scope(SymTable *st)
{
    /* Εκτύπωση συμβόλων τρέχουσας εμβέλειας */
    fprintf(stderr, "\n[Scope] Exiting depth %d - Symbols:\n", st->current_depth);
    fprintf(stderr, "%-20s %-12s %-12s %s\n", "Name", "Kind", "Type", "Depth");
    fprintf(stderr, "%-20s %-12s %-12s %s\n", "----", "----", "----", "-----");

    for (int i = 0; i < MAX_SYMBOLS; i++)
    {
        Symbol *s = st->table[i];
        while (s)
        {
            if (s->depth == st->current_depth)
            {
                fprintf(stderr, "%-20s %-12s %-12s %d\n",
                        s->name,
                        symkind_to_str(s->kind),
                        symtype_to_str(s->type),
                        s->depth);
            }
            s = s->next;
        }
    }
    st->current_depth--;
    fprintf(stderr, "[Scope] Back to depth %d\n\n", st->current_depth);
}

/*
 * symtable_insert - Εισαγωγή νέου συμβόλου στον ΠΣ
 *
 * Ελέγχει αν υπάρχει ήδη σύμβολο με το ίδιο όνομα στην
 * τρέχουσα εμβέλεια. Αν ναι, επιστρέφει NULL με μήνυμα σφάλματος.
 * Αν όχι, δημιουργεί νέα εγγραφή και την εισάγει στο hash table.
 *
 * Επιστρέφει: δείκτη στο νέο σύμβολο ή NULL αν υπάρχει conflict
 */
Symbol *symtable_insert(SymTable *st, const char *name,
                        SymKind kind, SymType type)
{
    if (!name || strlen(name) == 0)
        return NULL;

    /* Έλεγχος για διπλή δήλωση στην ίδια εμβέλεια */
    Symbol *existing = symtable_lookup_current(st, name);
    if (existing)
    {
        fprintf(stderr,
                "Semantic error: '%s' already declared in this scope (depth %d)\n",
                name, st->current_depth);
        return NULL;
    }

    /* Δημιουργία νέας εγγραφής */
    unsigned int h = hash(name);
    Symbol *s = (Symbol *)malloc(sizeof(Symbol));
    memset(s, 0, sizeof(Symbol));
    strncpy(s->name, name, MAX_NAME - 1);
    s->name[MAX_NAME - 1] = '\0';
    s->kind = kind;
    s->type = type;
    s->depth = st->current_depth;
    s->is_static = 0;

    /* Εισαγωγή στην αρχή της λίστας (chaining) */
    s->next = st->table[h];
    st->table[h] = s;

    return s;
}

/*
 * symtable_lookup - Αναζήτηση συμβόλου στον ΠΣ
 *
 * Ψάχνει σε όλες τις εμβέλειες και επιστρέφει το σύμβολο
 * με το μεγαλύτερο βάθος (πιο κοντινή δήλωση = shadowing).
 *
 * Επιστρέφει: δείκτη στο σύμβολο ή NULL αν δεν βρεθεί
 */
Symbol *symtable_lookup(SymTable *st, const char *name)
{
    unsigned int h = hash(name);
    Symbol *s = st->table[h];
    Symbol *best = NULL;

    /* Εύρεση συμβόλου με μεγαλύτερο depth (πιο κοντινή εμβέλεια) */
    while (s)
    {
        if (strcmp(s->name, name) == 0)
        {
            if (best == NULL || s->depth > best->depth)
                best = s;
        }
        s = s->next;
    }
    return best;
}

/*
 * symtable_lookup_current - Αναζήτηση μόνο στην τρέχουσα εμβέλεια
 * Χρησιμοποιείται για έλεγχο διπλής δήλωσης
 *
 * Επιστρέφει: δείκτη στο σύμβολο ή NULL αν δεν βρεθεί
 */
Symbol *symtable_lookup_current(SymTable *st, const char *name)
{
    unsigned int h = hash(name);
    Symbol *s = st->table[h];
    while (s)
    {
        if (strcmp(s->name, name) == 0 && s->depth == st->current_depth)
            return s;
        s = s->next;
    }
    return NULL;
}

/*
 * symtable_print_current_scope - Εκτύπωση τρέχουσας εμβέλειας
 * Εκτυπώνει όλα τα σύμβολα της τρέχουσας εμβέλειας
 */
void symtable_print_current_scope(SymTable *st)
{
    printf("\n[SymTable] Current scope (depth %d):\n", st->current_depth);
    printf("%-20s %-12s %-12s %s\n", "Name", "Kind", "Type", "Depth");
    printf("%-20s %-12s %-12s %s\n", "----", "----", "----", "-----");
    for (int i = 0; i < MAX_SYMBOLS; i++)
    {
        Symbol *s = st->table[i];
        while (s)
        {
            if (s->depth == st->current_depth)
            {
                printf("%-20s %-12s %-12s %d\n",
                       s->name,
                       symkind_to_str(s->kind),
                       symtype_to_str(s->type),
                       s->depth);
            }
            s = s->next;
        }
    }
}

/*
 * symkind_to_str - Μετατροπή SymKind σε string
 */
const char *symkind_to_str(SymKind kind)
{
    switch (kind)
    {
    case SYM_VARIABLE:
        return "VARIABLE";
    case SYM_FUNCTION:
        return "FUNCTION";
    case SYM_CONSTANT:
        return "CONSTANT";
    case SYM_TYPE:
        return "TYPE";
    case SYM_ENUM:
        return "ENUM";
    case SYM_CLASS:
        return "CLASS";
    case SYM_UNION:
        return "UNION";
    case SYM_PARAMETER:
        return "PARAMETER";
    default:
        return "UNKNOWN";
    }
}

/*
 * symtype_to_str - Μετατροπή SymType σε string
 */
const char *symtype_to_str(SymType type)
{
    switch (type)
    {
    case TYPE_INT:
        return "int";
    case TYPE_FLOAT:
        return "float";
    case TYPE_CHAR:
        return "char";
    case TYPE_STRING:
        return "string";
    case TYPE_VOID:
        return "void";
    case TYPE_ENUM:
        return "enum";
    case TYPE_CLASS:
        return "class";
    case TYPE_UNION:
        return "union";
    case TYPE_ARRAY:
        return "array";
    case TYPE_LIST:
        return "list";
    default:
        return "unknown";
    }
}