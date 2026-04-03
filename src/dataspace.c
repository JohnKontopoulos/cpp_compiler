#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dataspace.h"

DataSpace *dataspace_create()
{
    DataSpace *ds = (DataSpace *)calloc(1, sizeof(DataSpace));
    ds->count = 0;
    ds->global_offset = 0;
    ds->local_offset = 0;
    return ds;
}

void dataspace_destroy(DataSpace *ds)
{
    if (ds)
        free(ds);
}

/* Μέγεθος τύπου σε bytes (για MIPS) */
int dataspace_type_size(SymType type)
{
    switch (type)
    {
    case TYPE_INT:
        return 4;
    case TYPE_FLOAT:
        return 4;
    case TYPE_CHAR:
        return 1;
    case TYPE_STRING:
        return 256; /* max string size */
    default:
        return 4;
    }
}

DataEntry *dataspace_alloc(DataSpace *ds, const char *name,
                           SymType type, StorageClass storage, int depth)
{
    if (ds->count >= MAX_ENTRIES)
    {
        fprintf(stderr, "DataSpace full!\n");
        return NULL;
    }

    DataEntry *e = &ds->entries[ds->count++];
    strncpy(e->name, name, 255);
    e->type = type;
    e->storage = storage;
    e->depth = depth;
    e->size = dataspace_type_size(type);

    /* Υπολογισμός offset */
    if (storage == STORAGE_GLOBAL)
    {
        e->offset = ds->global_offset;
        ds->global_offset += e->size;
    }
    else
    {
        e->offset = ds->local_offset;
        ds->local_offset += e->size;
    }

    return e;
}

void dataspace_enter_scope(DataSpace *ds)
{
    ds->local_offset = 0; /* νέα εμβέλεια ξεκινά από 0 */
}

void dataspace_exit_scope(DataSpace *ds, int depth)
{
    /* Εκτύπωση ΧΔ για την εμβέλεια */
    dataspace_print_scope(ds, depth);

    /* Απελευθέρωση τοπικών μεταβλητών */
    int new_count = 0;
    for (int i = 0; i < ds->count; i++)
    {
        if (ds->entries[i].depth != depth ||
            ds->entries[i].storage == STORAGE_GLOBAL)
        {
            ds->entries[new_count++] = ds->entries[i];
        }
    }
    ds->count = new_count;
    ds->local_offset = 0;
}

void dataspace_print_scope(DataSpace *ds, int depth)
{
    printf("\n[DataSpace] Scope depth %d:\n", depth);
    printf("%-20s %-10s %-10s %-8s %s\n",
           "Name", "Type", "Storage", "Offset", "Size");
    printf("%-20s %-10s %-10s %-8s %s\n",
           "----", "----", "-------", "------", "----");

    int found = 0;
    for (int i = 0; i < ds->count; i++)
    {
        DataEntry *e = &ds->entries[i];
        if (e->depth == depth)
        {
            const char *storage_str =
                e->storage == STORAGE_GLOBAL ? "global" : e->storage == STORAGE_PARAM ? "param"
                                                                                      : "local";
            printf("%-20s %-10s %-10s %-8d %d\n",
                   e->name,
                   symtype_to_str(e->type),
                   storage_str,
                   e->offset,
                   e->size);
            found = 1;
        }
    }
    if (!found)
        printf("(empty)\n");
}

void dataspace_print(DataSpace *ds)
{
    printf("\n[DataSpace] All entries:\n");
    printf("%-20s %-10s %-10s %-8s %s\n",
           "Name", "Type", "Storage", "Offset", "Size");
    printf("%-20s %-10s %-10s %-8s %s\n",
           "----", "----", "-------", "------", "----");

    for (int i = 0; i < ds->count; i++)
    {
        DataEntry *e = &ds->entries[i];
        const char *storage_str =
            e->storage == STORAGE_GLOBAL ? "global" : e->storage == STORAGE_PARAM ? "param"
                                                                                  : "local";
        printf("%-20s %-10s %-10s %-8d %d\n",
               e->name,
               symtype_to_str(e->type),
               storage_str,
               e->offset,
               e->size);
    }
}