#ifndef SYMTABLE_H
#define SYMTABLE_H

#define MAX_NAME 256
#define MAX_SYMBOLS 1024

/* Κατηγορίες συμβόλων */
typedef enum
{
    SYM_VARIABLE,
    SYM_FUNCTION,
    SYM_CONSTANT,
    SYM_TYPE,
    SYM_ENUM,
    SYM_CLASS,
    SYM_UNION,
    SYM_PARAMETER
} SymKind;

/* Βασικοί τύποι */
typedef enum
{
    TYPE_INT,
    TYPE_FLOAT,
    TYPE_CHAR,
    TYPE_STRING,
    TYPE_VOID,
    TYPE_ENUM,
    TYPE_CLASS,
    TYPE_UNION,
    TYPE_ARRAY,
    TYPE_LIST,
    TYPE_UNKNOWN
} SymType;

/* Εγγραφή πίνακα συμβόλων */
typedef struct Symbol
{
    char name[MAX_NAME];
    SymKind kind;
    SymType type;
    int depth; /* βάθος εμβέλειας */
    int is_static;
    struct Symbol *next; /* για chaining στο hash table */
} Symbol;

/* Πίνακας Συμβόλων */
typedef struct
{
    Symbol *table[MAX_SYMBOLS];
    int current_depth;
} SymTable;

/* Συναρτήσεις */
SymTable *symtable_create();
void symtable_destroy(SymTable *st);

void symtable_enter_scope(SymTable *st);
void symtable_exit_scope(SymTable *st);

Symbol *symtable_insert(SymTable *st, const char *name, SymKind kind, SymType type);
Symbol *symtable_lookup(SymTable *st, const char *name);
Symbol *symtable_lookup_current(SymTable *st, const char *name);

void symtable_print_current_scope(SymTable *st);

const char *symkind_to_str(SymKind kind);
const char *symtype_to_str(SymType type);

#endif