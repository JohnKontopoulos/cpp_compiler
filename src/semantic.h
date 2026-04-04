/*
 * semantic.h
 * Σημασιολογικός Αναλυτής για τον μεταγλωττιστή CPP
 *
 * Παρέχει ελέγχους σημασιολογικής ορθότητας:
 * - Έλεγχος τύπων εκφράσεων
 * - Έλεγχος δηλώσεων και χρήσης αναγνωριστικών
 * - Αυτόματη εισαγωγή cast nodes για μετατροπές τύπων
 * - Έλεγχος αριθμού παραμέτρων κλήσεων συναρτήσεων
 * - Έλεγχος συνθηκών if/while (πρέπει να είναι int)
 */

#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "symtable.h"
#include "ast.h"

/* ==================== ΕΛΕΓΧΟΣ ΤΥΠΩΝ ==================== */

/*
 * semantic_check_expr - Σημασιολογικός έλεγχος έκφρασης
 * Ελέγχει τύπους και εισάγει cast nodes όπου χρειάζεται
 * Επιστρέφει: τύπος αποτελέσματος της έκφρασης
 */
SymType semantic_check_expr(ASTNode *node, SymTable *st);

/*
 * semantic_check_stmt - Σημασιολογικός έλεγχος εντολής
 * Ελέγχει σημασιολογική ορθότητα εντολών
 */
void semantic_check_stmt(ASTNode *node, SymTable *st);

/*
 * semantic_check_program - Κύριος σημασιολογικός έλεγχος
 * Καλείται μετά το parsing για έλεγχο ολόκληρου του προγράμματος
 */
void semantic_check_program(ASTNode *root, SymTable *st);

/* ==================== ΒΟΗΘΗΤΙΚΕΣ ==================== */

/*
 * types_compatible - Έλεγχος συμβατότητας τύπων
 * Δύο τύποι είναι συμβατοί αν είναι ίδιοι ή
 * αν είναι και οι δύο αριθμητικοί (int/float)
 * Επιστρέφει: 1 αν συμβατοί, 0 αν ασύμβατοι
 */
int types_compatible(SymType t1, SymType t2);

/*
 * result_type - Υπολογισμός τύπου αποτελέσματος
 * Για αριθμητικές πράξεις: float κυριαρχεί έναντι int
 * Επιστρέφει: τύπος αποτελέσματος
 */
SymType result_type(SymType t1, SymType t2);

/* ==================== ΜΗΝΥΜΑΤΑ ΣΦΑΛΜΑΤΩΝ ==================== */

/*
 * sem_error - Εκτύπωση μηνύματος σημασιολογικού σφάλματος
 * Αυξάνει τον μετρητή σφαλμάτων
 */
void sem_error(const char *msg, const char *name);

/*
 * sem_warning - Εκτύπωση προειδοποίησης (χωρίς τερματισμό)
 */
void sem_warning(const char *msg, const char *name);

/* Μετρητής σημασιολογικών σφαλμάτων */
extern int sem_error_count;

#endif