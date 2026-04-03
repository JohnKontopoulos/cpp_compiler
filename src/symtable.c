#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"

/* Απλή hash function */
static unsigned int hash(const char *name)
{
    unsigned int h = 0;
    while (*name)
        h = h * 31 + (unsigned char)*name++;
    return h % MAX_SYMBOLS;
}

SymTable *symtable_create()
{
    SymTable *st = (SymTable *)malloc(sizeof(SymTable));
    memset(st->table, 0, sizeof(st->table));
    st->current_depth = 0;
    return st;
}

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

void symtable_enter_scope(SymTable *st)
{
    st->current_depth++;
    printf("[Scope] Entering depth %d\n", st->current_depth);
}

void symtable_exit_scope(SymTable *st)
{
    printf("\n[Scope] Exiting depth %d - Symbols:\n", st->current_depth);
    printf("%-20s %-12s %-12s %s\n", "Name", "Kind", "Type", "Depth");
    printf("%-20s %-12s %-12s %s\n", "----", "----", "----", "-----");

    /* Εκτύπωση συμβόλων - ΔΕΝ διαγράφουμε */
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
    st->current_depth--;
    printf("[Scope] Back to depth %d\n\n", st->current_depth);
}

Symbol *symtable_insert(SymTable *st, const char *name, SymKind kind, SymType type)
{
    if (!name || strlen(name) == 0)
        return NULL;

    /* Έλεγχος αν υπάρχει ήδη στην ίδια εμβέλεια */
    Symbol *existing = symtable_lookup_current(st, name);
    if (existing)
    {
        fprintf(stderr, "Semantic error: '%s' already declared in this scope (depth %d)\n",
                name, st->current_depth);
        return NULL;
    }

    unsigned int h = hash(name);
    Symbol *s = (Symbol *)malloc(sizeof(Symbol));
    memset(s, 0, sizeof(Symbol));
    strncpy(s->name, name, MAX_NAME - 1);
    s->name[MAX_NAME - 1] = '\0';
    s->kind = kind;
    s->type = type;
    s->depth = st->current_depth;
    s->is_static = 0;
    s->next = st->table[h];
    st->table[h] = s;

    return s;
}

Symbol *symtable_lookup(SymTable *st, const char *name)
{
    unsigned int h = hash(name);
    Symbol *s = st->table[h];
    Symbol *best = NULL;

    /* Βρίσκουμε το σύμβολο με το μεγαλύτερο depth (πιο κοντινή εμβέλεια) */
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