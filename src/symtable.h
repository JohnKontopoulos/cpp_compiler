#ifndef SYMTABLE_H
#define SYMTABLE_H

#define MAX_NAME 256
#define MAX_SYMBOLS 1024
#define MAX_PARAMS 32

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

/* Παράμετρος συνάρτησης */
typedef struct
{
    char name[MAX_NAME];
    SymType type;
    int by_ref; /* 1 αν περνάει κατ' αναφορά */
    int is_array;
} ParamInfo;

/* Εγγραφή πίνακα συμβόλων */
typedef struct Symbol
{
    char name[MAX_NAME];
    SymKind kind;
    SymType type;
    int depth;
    int is_static;
    int is_const;

    /* Για συναρτήσεις */
    int param_count;
    ParamInfo params[MAX_PARAMS];
    int has_body; /* 1 αν έχει πλήρη δήλωση */

    /* Για πίνακες */
    int array_dims;
    int dim_sizes[8];

    /* Για σταθερές */
    int const_ival;
    float const_fval;
    char const_cval;
    char *const_sval;

    /* Για χώρο δεδομένων */
    int offset; /* μετατόπιση στον ΧΔ */

    struct Symbol *next;
} Symbol;

typedef struct
{
    Symbol *table[MAX_SYMBOLS];
    int current_depth;
} SymTable;

/* Βασικές συναρτήσεις */
SymTable *symtable_create();
void symtable_destroy(SymTable *st);
void symtable_enter_scope(SymTable *st);
void symtable_exit_scope(SymTable *st);
Symbol *symtable_insert(SymTable *st, const char *name, SymKind kind, SymType type);
Symbol *symtable_lookup(SymTable *st, const char *name);
Symbol *symtable_lookup_current(SymTable *st, const char *name);
void symtable_print_current_scope(SymTable *st);

/* Βοηθητικές */
const char *symkind_to_str(SymKind kind);
const char *symtype_to_str(SymType type);

#endif