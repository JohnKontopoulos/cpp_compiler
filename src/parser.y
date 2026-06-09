/*
 * parser.y
 * Συντακτικός και Σημασιολογικός Αναλυτής για τη γλώσσα CPP
 *
 * Χρησιμοποιεί το εργαλείο bison για την αυτόματη παραγωγή
 * του συντακτικού αναλυτή LALR(1).
 *
 * Χαρακτηριστικά:
 * - Επίλυση conflicts με προτεραιότητα/προσεταιριστικότητα τελεστών (μέθοδος β)
 * - Panic mode error recovery (μέχρι 5 σφάλματα)
 * - Κατασκευή ΑΣΔ κατά την ανάλυση
 * - Εισαγωγή συμβόλων στον Πίνακα Συμβόλων (ΠΣ)
 * - Δέσμευση χώρου στον Χώρο Δεδομένων (ΧΔ)
 * - Αποθήκευση συναρτήσεων για παραγωγή κώδικα
 */
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"
#include "ast.h"
#include "dataspace.h"
#include "codegen.h"

/* Ρίζα του Αφηρημένου Συντακτικού Δέντρου */
ASTNode *ast_root = NULL;

/* Εξωτερικές δηλώσεις από τον λεκτικό αναλυτή */
extern int yylex();
extern int yylineno;
extern char *yytext;
extern char current_line[];  /* Τρέχουσα γραμμή για μηνύματα σφαλμάτων */

/* Global codegen για αποθήκευση συναρτήσεων κατά το parsing */
extern CodeGen *global_cg;

void yyerror(const char *msg);

/* Μετρητής συντακτικών σφαλμάτων */
int error_count = 0;

/* Καθολικές δομές δεδομένων μεταγλωττιστή */
SymTable  *symtable;   /* Πίνακας Συμβόλων */
DataSpace *dataspace;  /* Χώρος Δεδομένων */

/* ==================== Καθολικές μεταβλητές κατάστασης ==================== */
/* Τρέχων τύπος κατά τη δήλωση μεταβλητών/παραμέτρων */
SymType current_type      = TYPE_UNKNOWN;
/* Σημαία για static δηλώσεις */
int     current_is_static = 0;
/* Σημαία: βρισκόμαστε σε λίστα παραμέτρων (για σωστή δέσμευση STORAGE_PARAM) */
int     in_parameter_list = 0;
/* Όνομα τρέχουσας συνάρτησης (για εισαγωγή παραμέτρων στον ΠΣ) */
char    current_func_name[256] = "";
/* Δείκτης τρέχουσας παραμέτρου */
int     current_param_index    = 0;
%}

/* ==================== ΤΥΠΟΙ ΚΑΤΗΓΟΡΗΜΑΤΩΝ ==================== */
%union {
    int      ival;   /* Ακέραια τιμή (ICONST, τύποι) */
    float    fval;   /* Πραγματική τιμή (FCONST) */
    char     cval;   /* Χαρακτήρας (CCONST) */
    char    *sval;   /* Συμβολοσειρά (ID, SCONST, LISTFUNC) */
    ASTNode *node;   /* Κόμβος ΑΣΔ */
}

/* ==================== ΤΥΠΟΙ ΜΗ-ΤΕΡΜΑΤΙΚΩΝ ==================== */
%type <ival> standard_type typename
%type <node> expression general_expression assignment
%type <node> constant variable
%type <node> optexpr
%type <node> expression_list listexpression
%type <node> main_function statements statement
%type <node> if_statement while_statement for_statement
%type <node> return_statement io_statement
%type <node> comp_statement expression_statement
%type <node> in_list in_item out_list out_item
%type <node> decl_statements

/* ==================== ΤΕΡΜΑΤΙΚΑ ΣΥΜΒΟΛΑ ==================== */
/* Λέξεις-κλειδιά */
%token TYPEDEF CHAR INT FLOAT STRING CONST CLASS
%token PRIVATE PROTECTED PUBLIC VOID STATIC UNION ENUM LIST
%token CONTINUE BREAK IF ELSE WHILE FOR RETURN LENGTH
%token CIN COUT MAIN THIS SIZEOP
/* Συναρτήσεις λίστας (CAR, CDR, κλπ) */
%token <sval> LISTFUNC
/* Τελεστές */
%token OROP ANDOP EQUOP RELOP ADDOP MULOP NOTOP INCDEC
/* Διαχωριστικά */
%token LPAREN RPAREN SEMI DOT COMMA ASSIGN COLON
%token LBRACK RBRACK REFER LBRACE RBRACE METH INP OUT
/* Σταθερές */
%token <ival> ICONST
%token <fval> FCONST
%token <cval> CCONST
%token <sval> SCONST
%token <sval> ID

/* ==================== ΠΡΟΤΕΡΑΙΟΤΗΤΑ ΤΕΛΕΣΤΩΝ ==================== */
/* Από χαμηλότερη προς υψηλότερη προτεραιότητα */
%right ASSIGN                           /* Ανάθεση (δεξιά προσεταιριστικότητα) */
%left COMMA                             /* Κόμμα */
%left OROP                              /* Λογικό OR */
%left ANDOP                             /* Λογικό AND */
%left EQUOP                             /* Ισότητα == != */
%left RELOP                             /* Σύγκριση < > <= >= */
%left ADDOP                             /* Πρόσθεση/αφαίρεση */
%left MULOP                             /* Πολλαπλασιασμός/διαίρεση */
%right NOTOP PREINCDEC UMINUS SIZEOP    /* Μοναδιαίοι (δεξιά) */
%left DOT LBRACK LPAREN POSTINCDEC      /* Postfix (υψηλότερη) */

/* Επίλυση dangling-else: το else συνδέεται με το κοντινότερο if */
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%start program

%%

/* ==================== ΚΑΝΟΝΕΣ ΓΡΑΜΜΑΤΙΚΗΣ ==================== */

/* Κορυφαίος κανόνας: πρόγραμμα = καθολικές δηλώσεις + main */
program
    : global_declarations main_function
        { 
            fprintf(stderr, "Program parsed successfully!\n");
            ast_root = $2;  /* Αποθήκευση ρίζας ΑΣΔ */
        }
    ;

/* Μηδέν ή περισσότερες καθολικές δηλώσεις */
global_declarations
    : global_declarations global_declaration
    | /* empty */
    ;

/* Τύποι καθολικών δηλώσεων */
global_declaration
    : typedef_declaration
    | const_declaration
    | enum_declaration
    | class_declaration
    | union_declaration
    | typename listspec ID
        {
            /* Καθολική μεταβλητή ή συνάρτηση */
            current_type = $1;
            strncpy(current_func_name, $3, 255);
            current_param_index = 0;
            in_parameter_list = 1;  /* Ενεργοποίηση για STORAGE_PARAM */
            /* Εισαγωγή στον ΠΣ ως συνάρτηση (θα αλλάξει αν είναι μεταβλητή) */
            Symbol *s = symtable_insert(symtable, $3, SYM_FUNCTION, (SymType)$1);
            if (s) {
                s->param_count = 0;
                fprintf(stderr, "[SymTable] function '%s' return_type=%s\n",
                        $3, symtype_to_str((SymType)$1));
            }
        }
      global_rest
        {
            in_parameter_list = 0;  /* Απενεργοποίηση μετά τη δήλωση */
        }
    ;

/* Ό,τι ακολουθεί μετά το typename ID στην καθολική εμβέλεια */
global_rest
    : dims initializer SEMI                          /* Καθολική μεταβλητή */
    | dims initializer COMMA init_variabledefs SEMI  /* Πολλαπλές καθολικές μεταβλητές */
    | LPAREN parameter_list RPAREN LBRACE            /* Ορισμός συνάρτησης με παραμέτρους */
        { symtable_enter_scope(symtable); dataspace_enter_scope(dataspace); }
      decl_statements RBRACE
        { 
            int d = symtable->current_depth;
            symtable_exit_scope(symtable);
            dataspace_exit_scope(dataspace, d);
            if ($6) {
                /* Εκτύπωση ΑΣΔ συνάρτησης */
                fprintf(stderr, "\n=== AST for function '%s' ===\n",
                        current_func_name);
                ASTNode *stmt = $6;
                while (stmt) { ast_print(stmt, 1); stmt = stmt->next; }
                /* Αποθήκευση για παραγωγή κώδικα */
                if (global_cg) {
                    ASTNode *body = ast_make_compound($6);
                    Symbol *s = symtable_lookup(symtable, current_func_name);
                    int pc = s ? s->param_count : 0;
                    codegen_add_function(global_cg, current_func_name, body, pc);
                }
            }
        }
    | LPAREN parameter_list RPAREN SEMI              /* Πρωτότυπο συνάρτησης */
    | LPAREN RPAREN LBRACE                           /* Ορισμός συνάρτησης χωρίς παραμέτρους */
        { symtable_enter_scope(symtable); dataspace_enter_scope(dataspace); }
      decl_statements RBRACE
        { 
            int d = symtable->current_depth;
            symtable_exit_scope(symtable);
            dataspace_exit_scope(dataspace, d);
            if ($5) {
                fprintf(stderr, "\n=== AST for function '%s' ===\n",
                        current_func_name);
                ASTNode *stmt = $5;
                while (stmt) { ast_print(stmt, 1); stmt = stmt->next; }
                if (global_cg) {
                    ASTNode *body = ast_make_compound($5);
                    codegen_add_function(global_cg, current_func_name, body, 0);
                }
            }
        }
    | LPAREN RPAREN SEMI                             /* Πρωτότυπο χωρίς παραμέτρους */
    | LPAREN parameter_types RPAREN SEMI             /* Πρωτότυπο με τύπους παραμέτρων */
    | METH ID LPAREN parameter_list RPAREN LBRACE    /* Μέθοδος κλάσης με παραμέτρους */
        { symtable_enter_scope(symtable); dataspace_enter_scope(dataspace); }
      decl_statements RBRACE
        { 
            int d = symtable->current_depth;
            symtable_exit_scope(symtable);
            dataspace_exit_scope(dataspace, d);
        }
    | METH ID LPAREN RPAREN LBRACE                   /* Μέθοδος κλάσης χωρίς παραμέτρους */
        { symtable_enter_scope(symtable); dataspace_enter_scope(dataspace); }
      decl_statements RBRACE
        { 
            int d = symtable->current_depth;
            symtable_exit_scope(symtable);
            dataspace_exit_scope(dataspace, d);
        }
    ;

/* ==================== TYPEDEF ==================== */
typedef_declaration
    : TYPEDEF typename listspec ID dims SEMI
        {
            /* Εισαγωγή συνώνυμου τύπου στον ΠΣ */
            Symbol *s = symtable_insert(symtable, $4, SYM_TYPE, (SymType)$2);
            if (s)
                fprintf(stderr, "[SymTable] typedef '%s' = type %s\n",
                        $4, symtype_to_str((SymType)$2));
        }
    ;

/* Τύπος: βασικός ή αναγνωριστικό (typedef) */
typename
    : standard_type { $$ = $1; }
    | ID            { $$ = TYPE_UNKNOWN; }  /* Τύπος από typedef */
    ;

/* Βασικοί τύποι της γλώσσας */
standard_type
    : CHAR   { $$ = TYPE_CHAR; }
    | INT    { $$ = TYPE_INT; }
    | FLOAT  { $$ = TYPE_FLOAT; }
    | STRING { $$ = TYPE_STRING; }
    | VOID   { $$ = TYPE_VOID; }
    ;

/* Προαιρετικό LIST prefix για τύπους λίστας */
listspec
    : LIST
    | /* empty */
    ;

/* Διαστάσεις πίνακα: [n][m]... */
dims
    : dims dim
    | /* empty */
    ;

dim
    : LBRACK ICONST RBRACK   /* Διάσταση με σταθερά */
    | LBRACK RBRACK          /* Άγνωστη διάσταση */
    ;

/* ==================== CONST ==================== */
const_declaration
    : CONST standard_type
        { current_type = $2; }  /* Αποθήκευση τύπου σταθεράς */
      constdefs SEMI
    | CONST ID
        { current_type = TYPE_UNKNOWN; }  /* Τύπος από typedef */
      constdefs SEMI
    ;

constdefs
    : constdefs COMMA constdef
    | constdef
    ;

constdef
    : ID dims ASSIGN init_value
        {
            /* Εισαγωγή σταθεράς στον ΠΣ με is_const=1 */
            Symbol *s = symtable_insert(symtable, $1, SYM_CONSTANT, current_type);
            if (s) {
                s->is_const = 1;
                fprintf(stderr, "[SymTable] const '%s' type=%s\n",
                        $1, symtype_to_str(current_type));
            }
        }
    ;

/* Αρχική τιμή: έκφραση ή λίστα τιμών */
init_value
    : expression
    | LBRACE init_values RBRACE  /* Αρχικοποίηση πίνακα */
    ;

init_values
    : init_values COMMA init_value
    | init_value
    ;

/* ==================== ENUM ==================== */
enum_declaration
    : ENUM ID
        { symtable_insert(symtable, $2, SYM_ENUM, TYPE_ENUM); }
      enum_body SEMI
    ;

enum_body
    : LBRACE id_list RBRACE
    ;

/* Λίστα αναγνωριστικών με προαιρετικές τιμές */
id_list
    : id_list COMMA ID initializer
    | ID initializer
    ;

/* Προαιρετική αρχικοποίηση */
initializer
    : ASSIGN init_value
    | /* empty */
    ;

/* ==================== CLASS ==================== */
class_declaration
    : CLASS ID
        { symtable_insert(symtable, $2, SYM_CLASS, TYPE_CLASS); }
      class_body SEMI
    ;

class_body
    : parent LBRACE members_methods RBRACE
    ;

/* Προαιρετική κληρονομικότητα */
parent
    : COLON ID   /* Κληρονομεί από άλλη κλάση */
    | /* empty */
    ;

members_methods
    : members_methods access member_or_method
    | access member_or_method
    ;

/* Επίπεδο πρόσβασης μελών */
access
    : PRIVATE COLON
    | PROTECTED COLON
    | PUBLIC COLON
    | /* empty */
    ;

member_or_method
    : member
    | method
    ;

member
    : var_declaration
    | anonymous_union
    ;

/* Δήλωση μεταβλητής: typename variabledefs; */
var_declaration
    : typename variabledefs SEMI
    ;

variabledefs
    : variabledefs COMMA variabledef
    | variabledef
    ;

/* Ορισμός μεταβλητής/παραμέτρου */
variabledef
    : listspec ID dims
        {
            /* Αν είμαστε σε parameter list → PARAMETER, αλλιώς VARIABLE */
            SymKind kind = in_parameter_list ? SYM_PARAMETER : SYM_VARIABLE;
            Symbol *s = symtable_insert(symtable, $2, kind, current_type);
            if (s && current_is_static) s->is_static = 1;

            /* Καθορισμός κατηγορίας αποθήκευσης */
            StorageClass sc;
            if (in_parameter_list)
                sc = STORAGE_PARAM;    /* Παράμετρος → στοίβα */
            else if (symtable->current_depth == 0 || current_is_static)
                sc = STORAGE_GLOBAL;   /* Καθολική/static → .data */
            else
                sc = STORAGE_LOCAL;    /* Τοπική → στοίβα */

            /* Δέσμευση χώρου και αποθήκευση offset */
            DataEntry *e = dataspace_alloc(dataspace, $2, current_type,
                                           sc, symtable->current_depth);
            if (s && e) s->offset = e->offset;
        }
    ;

anonymous_union
    : UNION union_body SEMI
    ;

union_body
    : LBRACE fields RBRACE
    ;

fields
    : fields field
    | field
    ;

field
    : var_declaration
    ;

/* Δήλωση μεθόδου κλάσης (μόνο πρωτότυπο) */
method
    : typename listspec ID LPAREN parameter_types RPAREN SEMI
    | typename listspec ID LPAREN RPAREN SEMI
    ;

/* ==================== UNION ==================== */
union_declaration
    : UNION ID
        { symtable_insert(symtable, $2, SYM_UNION, TYPE_UNION); }
      union_body SEMI
    ;

/* ==================== ΠΑΡΑΜΕΤΡΟΙ ==================== */
/* Τύποι παραμέτρων χωρίς ονόματα (για πρωτότυπα) */
parameter_types
    : parameter_types COMMA typename pass_list_dims
    | typename pass_list_dims
    ;

pass_list_dims
    : REFER          /* Παράμετρος by reference */
    | listspec dims  /* Παράμετρος by value */
    ;

/* Λίστα παραμέτρων με ονόματα (για ορισμούς συναρτήσεων) */
parameter_list
    : parameter_list COMMA typename
        { current_type = $3; }
      pass_variabledef
    | typename
        { current_type = $1; }
      pass_variabledef
    ;

/* Ορισμός παραμέτρου: by value ή by reference */
pass_variabledef
    : variabledef
        {
            /* Παράμετρος by value: εγγραφή στον ΠΣ */
            Symbol *func = symtable_lookup(symtable, current_func_name);
            if (func && current_param_index < MAX_PARAMS) {
                func->params[current_param_index].type   = current_type;
                func->params[current_param_index].by_ref = 0;
                func->param_count++;
                fprintf(stderr, "[SymTable] param %d: type=%s by_value\n",
                        current_param_index, symtype_to_str(current_type));
                current_param_index++;
            }
        }
    | REFER ID
        {
            /* Παράμετρος by reference: εισαγωγή ως SYM_PARAMETER */
            Symbol *s = symtable_insert(symtable, $2, SYM_PARAMETER, current_type);
            DataEntry *e = dataspace_alloc(dataspace, $2, current_type,
                                           STORAGE_PARAM, symtable->current_depth);
            if (s && e) s->offset = e->offset;

            /* Ενημέρωση πληροφοριών παραμέτρου στη συνάρτηση */
            Symbol *func = symtable_lookup(symtable, current_func_name);
            if (func && current_param_index < MAX_PARAMS) {
                strncpy(func->params[current_param_index].name, $2, MAX_NAME-1);
                func->params[current_param_index].type   = current_type;
                func->params[current_param_index].by_ref = 1;
                func->param_count++;
                fprintf(stderr, "[SymTable] param %d: '%s' type=%s by_ref\n",
                        current_param_index, $2, symtype_to_str(current_type));
                current_param_index++;
            }
            if (s) s->is_static = 0;
        }
    ;

/* ==================== ΚΑΘΟΛΙΚΕΣ ΜΕΤΑΒΛΗΤΕΣ ==================== */
/* Πολλαπλές δηλώσεις με κόμμα και αρχικοποίηση */
init_variabledefs
    : init_variabledefs COMMA init_variabledef
    | init_variabledef
    ;

init_variabledef
    : variabledef initializer
    ;

/* ==================== ΕΝΤΟΛΕΣ ==================== */
/* Συνδυασμός δηλώσεων και εντολών σε σώμα συνάρτησης */
decl_statements
    : declarations statements  { $$ = $2; }  /* Δηλώσεις + εντολές */
    | declarations             { $$ = NULL; } /* Μόνο δηλώσεις */
    | statements               { $$ = $1; }  /* Μόνο εντολές */
    | /* empty */              { $$ = NULL; } /* Κενό σώμα */
    ;

/* Δηλώσεις τοπικών μεταβλητών */
declarations
    : declarations STATIC standard_type
        { current_type = $3; current_is_static = 1; }
      variabledefs SEMI
        { current_is_static = 0; }
    | declarations standard_type
        { current_type = $2; current_is_static = 0; }
      variabledefs SEMI
    | STATIC standard_type
        { current_type = $2; current_is_static = 1; }
      variabledefs SEMI
        { current_is_static = 0; }
    | standard_type
        { current_type = $1; current_is_static = 0; }
      variabledefs SEMI
    ;

/* Λίστα εντολών */
statements
    : statements statement
        { $$ = ast_append($1, $2); }  /* Προσθήκη εντολής στη λίστα */
    | statement
        { $$ = $1; }
    ;

/* Τύποι εντολών */
statement
    : expression_statement  { $$ = $1; }
    | if_statement          { $$ = $1; }
    | while_statement       { $$ = $1; }
    | for_statement         { $$ = $1; }
    | return_statement      { $$ = $1; }
    | io_statement          { $$ = $1; }
    | comp_statement        { $$ = $1; }
    | CONTINUE SEMI         { $$ = ast_make_node_simple(NODE_CONTINUE); }
    | BREAK SEMI            { $$ = ast_make_node_simple(NODE_BREAK); }
    | SEMI                  { $$ = NULL; }  /* Κενή εντολή */
    | error SEMI            /* Ανάνηψη από σφάλμα (panic mode) */
        { 
            $$ = NULL;
            fprintf(stderr, "Error recovery at line %d\n>>> Line %d: %s\n",
                    yylineno, yylineno, current_line);
        }
    ;

/* Έκφραση ως εντολή */
expression_statement
    : general_expression SEMI
        { $$ = $1; }
    ;

/* ==================== ΔΟΜΕΣ ΕΛΕΓΧΟΥ ==================== */
/* if-then ή if-then-else */
/* %prec LOWER_THAN_ELSE: επίλυση dangling-else */
if_statement
    : IF LPAREN general_expression RPAREN statement
      %prec LOWER_THAN_ELSE
        { $$ = ast_make_if($3, $5, NULL); }
    | IF LPAREN general_expression RPAREN statement ELSE statement
        { $$ = ast_make_if($3, $5, $7); }
    ;

/* Βρόχος while */
while_statement
    : WHILE LPAREN general_expression RPAREN statement
        { $$ = ast_make_while($3, $5); }
    ;

/* Βρόχος for: init; cond; step */
for_statement
    : FOR LPAREN optexpr SEMI optexpr SEMI optexpr RPAREN statement
        { $$ = ast_make_for($3, $5, $7, $9); }
    ;

/* Προαιρετική έκφραση (για for) */
optexpr
    : general_expression  { $$ = $1; }
    | /* empty */         { $$ = NULL; }
    ;

/* Εντολή επιστροφής */
return_statement
    : RETURN optexpr SEMI
        { $$ = ast_make_return($2); }
    ;

/* ==================== ΕΙΣΟΔΟΣ/ΕΞΟΔΟΣ ==================== */
io_statement
    : CIN INP in_list SEMI
        { $$ = ast_make_cin($3); }   /* cin >> x >> y */
    | COUT OUT out_list SEMI
        { $$ = ast_make_cout($3); }  /* cout << x << y */
    ;

/* Λίστα μεταβλητών για cin */
in_list
    : in_list INP in_item
        { $1->next = $3; $$ = $1; }
    | in_item
        { $$ = $1; }
    ;

in_item
    : variable  { $$ = $1; }
    ;

/* Λίστα εκφράσεων για cout */
out_list
    : out_list OUT out_item
        { $1->next = $3; $$ = $1; }
    | out_item
        { $$ = $1; }
    ;

out_item
    : general_expression  { $$ = $1; }
    ;

/* Σύνθετη εντολή: νέα εμβέλεια */
comp_statement
    : LBRACE
        { symtable_enter_scope(symtable); dataspace_enter_scope(dataspace); }
      decl_statements RBRACE
        {
            int d = symtable->current_depth;
            symtable_exit_scope(symtable);
            dataspace_exit_scope(dataspace, d);
            $$ = ast_make_compound($3);
        }
    ;

/* ==================== MAIN ==================== */
main_function
    : main_header LBRACE
        { symtable_enter_scope(symtable); dataspace_enter_scope(dataspace); }
      decl_statements RBRACE
        { 
            int d = symtable->current_depth;
            symtable_exit_scope(symtable);
            dataspace_exit_scope(dataspace, d);
            $$ = ast_make_compound($4);
        }
    ;

/* Υπογραφή της main */
main_header
    : INT MAIN LPAREN RPAREN
    ;

/* ==================== ΕΚΦΡΑΣΕΙΣ ==================== */
/* Γενική έκφραση: περιλαμβάνει τελεστή κόμμα */
general_expression
    : general_expression COMMA general_expression
        { $$ = ast_make_binop(",", $1, $3); }
    | assignment
        { $$ = $1; }
    ;

/* Ανάθεση: δεξιά προσεταιριστικότητα */
assignment
    : variable ASSIGN assignment
        { $$ = ast_make_assign($1, $3); }
    | expression
        { $$ = $1; }
    ;

/* Εκφράσεις με τελεστές */
expression
    : expression OROP expression
        { $$ = ast_make_binop("||", $1, $3); }
    | expression ANDOP expression
        { $$ = ast_make_binop("&&", $1, $3); }
    | expression EQUOP expression
        { $$ = ast_make_binop("==", $1, $3); }
    | expression RELOP expression
        { $$ = ast_make_binop("<>", $1, $3); }
    | expression ADDOP expression
        { $$ = ast_make_binop("+", $1, $3); }
    | expression MULOP expression
        { $$ = ast_make_binop("*", $1, $3); }
    | NOTOP expression
        { $$ = ast_make_unop("!", $2); }
    | ADDOP expression %prec UMINUS
        { $$ = ast_make_unop("-", $2); }       /* Unary minus */
    | SIZEOP expression
        { $$ = ast_make_unop("sizeof", $2); }
    | INCDEC variable %prec PREINCDEC
        { $$ = ast_make_unop("pre++", $2); }   /* Pre-increment/decrement */
    | variable INCDEC %prec POSTINCDEC
        { $$ = ast_make_unop("post++", $1); }  /* Post-increment/decrement */
    | variable
        { $$ = $1; }
    | variable LPAREN expression_list RPAREN
        { $$ = ast_make_call($1->name, $3); }  /* Κλήση συνάρτησης */
    | LENGTH LPAREN general_expression RPAREN
        { $$ = ast_make_call("length", $3); }  /* Εύρεση μήκους λίστας */
    | constant
        { $$ = $1; }
    | LPAREN general_expression RPAREN
        { $$ = $2; }                           /* Παρενθέσεις */
    | LPAREN standard_type RPAREN
        { $$ = ast_make_iconst($2); }          /* Cast έκφραση */
    | listexpression
        { $$ = $1; }
    ;

/* Μεταβλητή: αναγνωριστικό με δυνατότητα indexing και field access */
variable
    : variable LBRACK general_expression RBRACK
        { $$ = ast_make_binop("[]", $1, $3); }        /* Στοιχείο πίνακα */
    | variable DOT ID
        { $$ = ast_make_binop(".", $1, ast_make_id($3, TYPE_UNKNOWN)); } /* Πεδίο */
    | LISTFUNC LPAREN general_expression RPAREN
        { $$ = ast_make_call($1, $3); }               /* Συνάρτηση λίστας */
    | ID
        {
            /* Αναζήτηση τύπου στον ΠΣ */
            Symbol *s = symtable_lookup(symtable, $1);
            SymType t = s ? s->type : TYPE_UNKNOWN;
            $$ = ast_make_id($1, t);
        }
    | THIS
        { $$ = ast_make_id("this", TYPE_UNKNOWN); }   /* Αναφορά στο τρέχον αντικείμενο */
    ;

/* Λίστα ορισμάτων κλήσης συνάρτησης */
expression_list
    : general_expression  { $$ = $1; }
    | /* empty */         { $$ = NULL; }
    ;

/* Σταθερές τιμές */
constant
    : CCONST  { $$ = ast_make_cconst($1); }  /* Χαρακτήρας */
    | ICONST  { $$ = ast_make_iconst($1); }  /* Ακέραιος */
    | FCONST  { $$ = ast_make_fconst($1); }  /* Πραγματικός */
    | SCONST  { $$ = ast_make_sconst($1); }  /* Συμβολοσειρά */
    ;

/* Έκφραση λίστας: [expr] */
listexpression
    : LBRACK expression_list RBRACK
        { $$ = $2; }
    ;

%%

/*
 * yyerror - Χειρισμός συντακτικών σφαλμάτων
 *
 * Καλείται αυτόματα από το bison όταν εντοπίσει συντακτικό σφάλμα.
 * Εκτυπώνει μήνυμα με αριθμό γραμμής και περιεχόμενο γραμμής.
 * Η ανάνηψη γίνεται με panic mode (κανόνας error SEMI).
 * Τερματίζει μετά από 5 σφάλματα.
 */
void yyerror(const char *msg) {
    fprintf(stderr, "Syntax error at line %d: %s\n", yylineno, msg);
    fprintf(stderr, ">>> Line %d: %s\n", yylineno, current_line);
    error_count++;
    if (error_count >= 5) {
        fprintf(stderr, "Too many errors (5), aborting.\n");
        exit(1);
    }
}