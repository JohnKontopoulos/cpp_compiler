/*
 * dataspace.h
 * Χώρος Δεδομένων (ΧΔ) για τον μεταγλωττιστή CPP
 *
 * Διαχειρίζεται τη δέσμευση θέσεων μνήμης για τις μεταβλητές
 * του προγράμματος και υπολογίζει τα offsets τους.
 *
 * Κατηγορίες αποθήκευσης:
 * - GLOBAL: καθολικές/static μεταβλητές → τμήμα .data (MIPS $gp)
 * - LOCAL:  τοπικές μεταβλητές → στοίβα (MIPS $sp)
 * - PARAM:  παράμετροι συνάρτησης → στοίβα (MIPS $sp/$a0-$a3)
 *
 * Μεγέθη τύπων (MIPS):
 * - int:    4 bytes
 * - float:  4 bytes
 * - char:   1 byte
 * - string: 256 bytes (μέγιστο)
 */

#ifndef DATASPACE_H
#define DATASPACE_H

#include "symtable.h"

/* Μέγιστος αριθμός εγγραφών στον ΧΔ */
#define MAX_ENTRIES 1024

/* ==================== ΚΑΤΗΓΟΡΙΕΣ ΑΠΟΘΗΚΕΥΣΗΣ ==================== */
typedef enum
{
    STORAGE_GLOBAL, /* Καθολικές/στατικές → .data section */
    STORAGE_LOCAL,  /* Τοπικές → στοίβα ($sp) */
    STORAGE_PARAM   /* Παράμετροι → στοίβα ($sp/$a0-$a3) */
} StorageClass;

/* ==================== ΕΓΓΡΑΦΗ ΧΔ ==================== */
/* Κάθε μεταβλητή αντιστοιχεί σε μία εγγραφή ΧΔ */
typedef struct DataEntry
{
    char name[256];       /* Όνομα μεταβλητής */
    SymType type;         /* Τύπος δεδομένων */
    StorageClass storage; /* Κατηγορία αποθήκευσης */
    int offset;           /* Μετατόπιση από αρχή χώρου δεδομένων */
    int size;             /* Μέγεθος σε bytes */
    int depth;            /* Βάθος εμβέλειας */
} DataEntry;

/* ==================== ΧΩΡΟΣ ΔΕΔΟΜΕΝΩΝ ==================== */
typedef struct
{
    DataEntry entries[MAX_ENTRIES]; /* Πίνακας εγγραφών */
    int count;                      /* Πλήθος εγγραφών */
    int global_offset;              /* Τρέχον offset καθολικού ΧΔ */
    int local_offset;               /* Τρέχον offset τοπικού ΧΔ */
} DataSpace;

/* ==================== ΔΙΕΠΑΦΗ ==================== */
/* Δημιουργία/καταστροφή */
DataSpace *dataspace_create();
void dataspace_destroy(DataSpace *ds);

/* Υπολογισμός μεγέθους τύπου σε bytes */
int dataspace_type_size(SymType type);

/* Δέσμευση χώρου για μεταβλητή */
DataEntry *dataspace_alloc(DataSpace *ds, const char *name,
                           SymType type, StorageClass storage, int depth);

/* Διαχείριση εμβελειών */
void dataspace_enter_scope(DataSpace *ds);
void dataspace_exit_scope(DataSpace *ds, int depth);

/* Εκτύπωση */
void dataspace_print(DataSpace *ds);
void dataspace_print_scope(DataSpace *ds, int depth);

#endif