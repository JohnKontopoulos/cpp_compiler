# Ανάλυση Conflicts Συντακτικού Αναλυτή

## Γλώσσα: CPP - Χρήση bison

## Μέθοδος Επίλυσης

Χρησιμοποιήθηκε η μέθοδος **(β)** — χρήση **προτεραιότητας
και προσεταιριστικότητας** των τελεστών, χωρίς μετασχηματισμό
της γραμματικής.

## Δήλωση Προτεραιότητας στο parser.y

Οι τελεστές δηλώνονται από τη **χαμηλότερη** προς την
**υψηλότερη** προτεραιότητα:

```
%right ASSIGN                          /* χαμηλότερη */
%left  COMMA
%left  OROP
%left  ANDOP
%left  EQUOP
%left  RELOP
%left  ADDOP
%left  MULOP
%right NOTOP PREINCDEC UMINUS SIZEOP
%left  DOT LBRACK LPAREN POSTINCDEC   /* υψηλότερη */
```

## Conflicts Πριν την Επίλυση

### 1. State 17 — shift/reduce και reduce/reduce conflict

**Εμπλεκόμενοι κανόνες:**

- `func_header_start: typename • listspec ID`
- `global_var_declaration: typename • init_variabledefs`
- `class_func_header_start: typename • listspec func_class ID`

**Αιτία:** Όταν ο parser βλέπει `typename` στην καθολική
εμβέλεια, δεν μπορεί να αποφασίσει αν ακολουθεί:

- Δήλωση μεταβλητής: `int x;`
- Δήλωση συνάρτησης: `int foo(...)`
- Δήλωση μεθόδου: `int Class::foo(...)`

**Εμπλεκόμενα στοιχεία ΣΑ:**

- Token: `LIST`, `ID`
- Κανόνες: `listspec → ε`, `global_var_declaration → typename • ...`

### 2. State 181 — reduce/reduce conflict

**Εμπλεκόμενοι κανόνες:**

- `declarations: error SEMI •`
- `statement: error SEMI •`

**Αιτία:** Και οι δύο κανόνες έχουν `error SEMI` οπότε
ο parser δεν ξέρει ποιον να εφαρμόσει μετά από σφάλμα.

**Εμπλεκόμενα tokens:** CONTINUE, BREAK, IF, WHILE, FOR,
RETURN, CIN, COUT, THIS, SIZEOP, LISTFUNC

## Επίλυση Conflicts

### Conflict State 17

**Λύση:** Ενοποίηση `global_var_declaration` και
`func_declaration` σε έναν κανόνα `global_rest`:

```
global_declaration
    : typename listspec ID global_rest
    ;

global_rest
    : dims initializer SEMI              /* μεταβλητή */
    | LPAREN parameter_list RPAREN LBRACE ... /* συνάρτηση */
    | LPAREN RPAREN LBRACE ...           /* συνάρτηση χωρίς παραμέτρους */
    | LPAREN parameter_types RPAREN SEMI /* πρωτότυπο */
    ;
```

Έτσι ο parser διαβάζει `typename listspec ID` και μετά
αποφασίζει βάσει του επόμενου token.

### Conflict State 181

**Λύση:** Αφαίρεση του `error SEMI` από τον κανόνα
`declarations` — κρατήθηκε μόνο στο `statement`:

```
statement
    : ...
    | error SEMI  /* error recovery μόνο εδώ */
    ;
```

## Κατάσταση Μετά την Επίλυση

```
$ bison -d -v -o build/parser.tab.c src/parser.y 2>&1
# Κανένα conflict message — 0 conflicts
```

## Dangling Else

Το κλασικό πρόβλημα dangling-else επιλύεται με:

```
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
```

**Παράδειγμα:**

```cpp
if (a) if (b) x = 1; else x = 2;
```

Το `else` συνδέεται με το **εσωτερικό** `if`, όπως
απαιτείται από τη σημασιολογία της CPP.
