/*
 * symtable.h
 * Πίνακας Συμβόλων (ΠΣ) για τον μεταγλωττιστή CPP
 *
 * Υλοποίηση με hash table και chaining για επίλυση συγκρούσεων.
 * Υποστηρίζει εμβέλειες (scopes) με βάθος φωλιάσματος.
 *
 * Χαρακτηριστικά:
 * - Εισαγωγή/αναζήτηση συμβόλων σε O(1) μέσο χρόνο
 * - Διαχείριση εμβελειών με enter/exit scope
 * - Αποθήκευση πληροφοριών για μεταβλητές, συναρτήσεις,
 *   σταθερές, τύπους, enums, κλάσεις, unions, παραμέτρους
 * - Αποθήκευση offset για παραγωγή κώδικα
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

/* ==================== ΣΤΑΘΕΡΕΣ ==================== */
#define MAX_NAME 256     /* Μέγιστο μήκος ονόματος συμβόλου */
#define MAX_SYMBOLS 1024 /* Μέγεθος hash table */
#define MAX_PARAMS 32    /* Μέγιστος αριθμός παραμέτρων συνάρτησης */

/* ==================== ΤΥΠΟΙ ΣΥΜΒΟΛΩΝ ==================== */
/* Είδος συμβόλου στον ΠΣ */
typedef enum
{
    SYM_VARIABLE, /* Μεταβλητή */
    SYM_FUNCTION, /* Συνάρτηση */
    SYM_CONSTANT, /* Σταθερά (const) */
    SYM_TYPE,     /* Συνώνυμο τύπου (typedef) */
    SYM_ENUM,     /* Απαρίθμηση (enum) */
    SYM_CLASS,    /* Κλάση */
    SYM_UNION,    /* Ένωση (union) */
    SYM_PARAMETER /* Παράμετρος συνάρτησης */
} SymKind;

/* Τύπος δεδομένων συμβόλου */
typedef enum
{
    TYPE_INT,    /* Ακέραιος */
    TYPE_FLOAT,  /* Πραγματικός */
    TYPE_CHAR,   /* Χαρακτήρας */
    TYPE_STRING, /* Συμβολοσειρά */
    TYPE_VOID,   /* Κενός τύπος */
    TYPE_ENUM,   /* Τύπος απαρίθμησης */
    TYPE_CLASS,  /* Τύπος κλάσης */
    TYPE_UNION,  /* Τύπος ένωσης */
    TYPE_ARRAY,  /* Πίνακας */
    TYPE_LIST,   /* Λίστα */
    TYPE_UNKNOWN /* Άγνωστος τύπος */
} SymType;

/* ==================== ΠΑΡΑΜΕΤΡΟΣ ΣΥΝΑΡΤΗΣΗΣ ==================== */
/* Πληροφορίες για κάθε παράμετρο συνάρτησης */
typedef struct
{
    char name[MAX_NAME]; /* Όνομα παραμέτρου */
    SymType type;        /* Τύπος παραμέτρου */
    int by_ref;          /* 1 αν περνάει κατ' αναφορά (&) */
    int is_array;        /* 1 αν είναι πίνακας */
} ParamInfo;

/* ==================== ΕΓΓΡΑΦΗ ΠΙΝΑΚΑ ΣΥΜΒΟΛΩΝ ==================== */
/* Κάθε σύμβολο αποθηκεύεται ως εγγραφή στον ΠΣ */
typedef struct Symbol
{
    char name[MAX_NAME]; /* Όνομα συμβόλου */
    SymKind kind;        /* Είδος συμβόλου */
    SymType type;        /* Τύπος δεδομένων */
    int depth;           /* Βάθος εμβέλειας (0=καθολική) */
    int is_static;       /* 1 αν δηλώθηκε ως static */
    int is_const;        /* 1 αν δηλώθηκε ως const */

    /* Πληροφορίες για συναρτήσεις */
    int param_count;              /* Αριθμός παραμέτρων */
    ParamInfo params[MAX_PARAMS]; /* Πληροφορίες παραμέτρων */
    int has_body;                 /* 1 αν έχει σώμα (όχι μόνο πρωτότυπο) */

    /* Πληροφορίες για πίνακες */
    int array_dims;   /* Αριθμός διαστάσεων */
    int dim_sizes[8]; /* Μέγεθος κάθε διάστασης */

    /* Τιμές σταθερών */
    int const_ival;   /* Τιμή ακέραιας σταθεράς */
    float const_fval; /* Τιμή πραγματικής σταθεράς */
    char const_cval;  /* Τιμή σταθεράς χαρακτήρα */
    char *const_sval; /* Τιμή σταθεράς συμβολοσειράς */

    /* Για παραγωγή κώδικα */
    int offset; /* Μετατόπιση από αρχή χώρου δεδομένων */

    struct Symbol *next; /* Επόμενο σύμβολο (chaining για hash conflicts) */
} Symbol;

/* ==================== ΠΙΝΑΚΑΣ ΣΥΜΒΟΛΩΝ ==================== */
typedef struct
{
    Symbol *table[MAX_SYMBOLS]; /* Hash table με chaining */
    int current_depth;          /* Τρέχον βάθος εμβέλειας */
} SymTable;

/* ==================== ΔΙΕΠΑΦΗ ==================== */
/* Δημιουργία/καταστροφή */
SymTable *symtable_create();
void symtable_destroy(SymTable *st);

/* Διαχείριση εμβελειών */
void symtable_enter_scope(SymTable *st);
void symtable_exit_scope(SymTable *st);

/* Εισαγωγή και αναζήτηση συμβόλων */
Symbol *symtable_insert(SymTable *st, const char *name,
                        SymKind kind, SymType type);
Symbol *symtable_lookup(SymTable *st, const char *name);
Symbol *symtable_lookup_current(SymTable *st, const char *name);

/* Εκτύπωση */
void symtable_print_current_scope(SymTable *st);

/* Μετατροπή σε string για εκτύπωση */
const char *symkind_to_str(SymKind kind);
const char *symtype_to_str(SymType type);

#endif