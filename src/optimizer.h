/*
 * optimizer.h
 * Βελτιστοποιητής Ενδιάμεσου Κώδικα για τον μεταγλωττιστή CPP
 *
 * Υλοποιεί τρεις βελτιστοποιήσεις στο επίπεδο του ΑΣΔ:
 *
 * #1 Αποτίμηση Σταθερών (Constant Folding):
 *    Υπολογίζει εκφράσεις με σταθερές κατά τη μεταγλώττιση.
 *    Π.χ. 2+3 → 5, x*0 → 0, x+0 → x
 *
 * #3 Απαλοιφή Άχρηστου Κώδικα (Dead Code Elimination):
 *    Αφαιρεί κώδικα που δεν εκτελείται ποτέ.
 *    Π.χ. κώδικας μετά από return, if(false), while(false)
 *
 * #4 Διάδοση Αντιγράφου (Copy Propagation):
 *    Αντικαθιστά μεταβλητές με τις τιμές τους όπου είναι γνωστές.
 *    Π.χ. x=5; y=x → x=5; y=5
 */

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "ast.h"

/*
 * optimize - Κύρια συνάρτηση βελτιστοποίησης
 * Εφαρμόζει με σειρά: copy propagation → constant folding → dead code elimination
 * Επιστρέφει: βελτιστοποιημένη ρίζα ΑΣΔ
 */
ASTNode *optimize(ASTNode *root);

/*
 * constant_folding - Αποτίμηση σταθερών εκφράσεων (#1)
 * Αναδρομική εφαρμογή σε όλο το ΑΣΔ
 * Επιστρέφει: βελτιστοποιημένος κόμβος
 */
ASTNode *constant_folding(ASTNode *node);

/*
 * dead_code_elimination - Απαλοιφή άχρηστου κώδικα (#3)
 * Αναδρομική εφαρμογή σε όλο το ΑΣΔ
 * Επιστρέφει: βελτιστοποιημένος κόμβος
 */
ASTNode *dead_code_elimination(ASTNode *node);

/*
 * copy_propagation - Διάδοση αντιγράφου (#4)
 * Αναδρομική εφαρμογή σε όλο το ΑΣΔ
 * Επιστρέφει: βελτιστοποιημένος κόμβος
 */
ASTNode *copy_propagation(ASTNode *node);

#endif