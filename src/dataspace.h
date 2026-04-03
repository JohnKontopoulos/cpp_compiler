#ifndef DATASPACE_H
#define DATASPACE_H

#include "symtable.h"

#define MAX_ENTRIES 1024

/* Κατηγορίες αποθήκευσης */
typedef enum
{
    STORAGE_GLOBAL, /* καθολικές/στατικές μεταβλητές */
    STORAGE_LOCAL,  /* τοπικές μεταβλητές στοίβας */
    STORAGE_PARAM   /* παράμετροι συνάρτησης */
} StorageClass;

/* Εγγραφή ΧΔ */
typedef struct DataEntry
{
    char name[256];
    SymType type;
    StorageClass storage;
    int offset; /* μετατόπιση από αρχή ΧΔ */
    int size;   /* μέγεθος σε bytes */
    int depth;  /* βάθος εμβέλειας */
} DataEntry;

/* Χώρος Δεδομένων */
typedef struct
{
    DataEntry entries[MAX_ENTRIES];
    int count;
    int global_offset; /* τρέχουσα θέση στον καθολικό ΧΔ */
    int local_offset;  /* τρέχουσα θέση στον τοπικό ΧΔ */
} DataSpace;

/* Συναρτήσεις */
DataSpace *dataspace_create();
void dataspace_destroy(DataSpace *ds);

int dataspace_type_size(SymType type);
DataEntry *dataspace_alloc(DataSpace *ds, const char *name,
                           SymType type, StorageClass storage, int depth);

void dataspace_enter_scope(DataSpace *ds);
void dataspace_exit_scope(DataSpace *ds, int depth);

void dataspace_print(DataSpace *ds);
void dataspace_print_scope(DataSpace *ds, int depth);

#endif