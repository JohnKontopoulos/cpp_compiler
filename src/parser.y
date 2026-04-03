%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"
#include "ast.h"
#include "dataspace.h"

ASTNode *ast_root = NULL;

extern int yylex();
extern int yylineno;
extern char *yytext;

void yyerror(const char *msg);
int error_count = 0;

SymTable *symtable;
DataSpace *dataspace;

SymType current_type = TYPE_UNKNOWN;
int current_is_static = 0;
%}

%union {
    int      ival;
    float    fval;
    char     cval;
    char    *sval;
    ASTNode *node;
}

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

/* Tokens */
%token TYPEDEF CHAR INT FLOAT STRING CONST CLASS
%token PRIVATE PROTECTED PUBLIC VOID STATIC UNION ENUM LIST
%token CONTINUE BREAK IF ELSE WHILE FOR RETURN LENGTH
%token CIN COUT MAIN THIS SIZEOP
%token <sval> LISTFUNC
%token OROP ANDOP EQUOP RELOP ADDOP MULOP NOTOP INCDEC
%token LPAREN RPAREN SEMI DOT COMMA ASSIGN COLON
%token LBRACK RBRACK REFER LBRACE RBRACE METH INP OUT
%token <ival> ICONST
%token <fval> FCONST
%token <cval> CCONST
%token <sval> SCONST
%token <sval> ID

/* Προτεραιότητα τελεστών */
%right ASSIGN
%left COMMA
%left OROP
%left ANDOP
%left EQUOP
%left RELOP
%left ADDOP
%left MULOP
%right NOTOP PREINCDEC UMINUS SIZEOP
%left DOT LBRACK LPAREN POSTINCDEC

/* Επίλυση dangling else */
%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%start program

%%

/* ==================== ΠΡΟΓΡΑΜΜΑ ==================== */
program
    : global_declarations main_function
        { 
            fprintf(stderr, "Program parsed successfully!\n");
            ast_root = $2;
        }
    ;

global_declarations
    : global_declarations global_declaration
    | /* empty */
    ;

global_declaration
    : typedef_declaration
    | const_declaration
    | enum_declaration
    | class_declaration
    | union_declaration
    | typename listspec ID global_rest
    ;

/* ==================== GLOBAL REST ==================== */
global_rest
    : dims initializer SEMI
    | dims initializer COMMA init_variabledefs SEMI
    | LPAREN parameter_list RPAREN LBRACE
        { symtable_enter_scope(symtable); dataspace_enter_scope(dataspace); }
      decl_statements RBRACE
        { 
            int d = symtable->current_depth;
            symtable_exit_scope(symtable);
            dataspace_exit_scope(dataspace, d);
            if ($6) {
                printf("\n=== AST for function ===\n");
                ASTNode *stmt = $6;
                while (stmt) { ast_print(stmt, 1); stmt = stmt->next; }
            }
        }
    | LPAREN parameter_list RPAREN SEMI
    | LPAREN RPAREN LBRACE
        { symtable_enter_scope(symtable); dataspace_enter_scope(dataspace); }
      decl_statements RBRACE
        { 
            int d = symtable->current_depth;
            symtable_exit_scope(symtable);
            dataspace_exit_scope(dataspace, d);
            if ($5) {
                printf("\n=== AST for function ===\n");
                ASTNode *stmt = $5;
                while (stmt) { ast_print(stmt, 1); stmt = stmt->next; }
            }
        }
    | LPAREN RPAREN SEMI
    | LPAREN parameter_types RPAREN SEMI
    | METH ID LPAREN parameter_list RPAREN LBRACE
        { symtable_enter_scope(symtable); dataspace_enter_scope(dataspace); }
      decl_statements RBRACE
        { 
            int d = symtable->current_depth;
            symtable_exit_scope(symtable);
            dataspace_exit_scope(dataspace, d);
        }
    | METH ID LPAREN RPAREN LBRACE
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
    ;

typename
    : standard_type { $$ = $1; }
    | ID            { $$ = TYPE_UNKNOWN; }
    ;

standard_type
    : CHAR   { $$ = TYPE_CHAR; }
    | INT    { $$ = TYPE_INT; }
    | FLOAT  { $$ = TYPE_FLOAT; }
    | STRING { $$ = TYPE_STRING; }
    | VOID   { $$ = TYPE_VOID; }
    ;

listspec
    : LIST
    | /* empty */
    ;

dims
    : dims dim
    | /* empty */
    ;

dim
    : LBRACK ICONST RBRACK
    | LBRACK RBRACK
    ;

/* ==================== CONST ==================== */
const_declaration
    : CONST typename constdefs SEMI
    ;

constdefs
    : constdefs COMMA constdef
    | constdef
    ;

constdef
    : ID dims ASSIGN init_value
    ;

init_value
    : expression
    | LBRACE init_values RBRACE
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

id_list
    : id_list COMMA ID initializer
    | ID initializer
    ;

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

parent
    : COLON ID
    | /* empty */
    ;

members_methods
    : members_methods access member_or_method
    | access member_or_method
    ;

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

var_declaration
    : typename variabledefs SEMI
    ;

variabledefs
    : variabledefs COMMA variabledef
    | variabledef
    ;

variabledef
    : listspec ID dims
        {
            Symbol *s = symtable_insert(symtable, $2, SYM_VARIABLE, current_type);
            if (s && current_is_static)
                s->is_static = 1;

            StorageClass sc = (symtable->current_depth == 0 || current_is_static)
                              ? STORAGE_GLOBAL : STORAGE_LOCAL;
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
parameter_types
    : parameter_types COMMA typename pass_list_dims
    | typename pass_list_dims
    ;

pass_list_dims
    : REFER
    | listspec dims
    ;

parameter_list
    : parameter_list COMMA typename pass_variabledef
        { current_type = $3; }
    | typename pass_variabledef
        { current_type = $1; }
    ;

pass_variabledef
    : variabledef
    | REFER ID
        {
            Symbol *s = symtable_insert(symtable, $2, SYM_PARAMETER, current_type);
            if (s) s->is_static = 0;
        }
    ;

/* ==================== ΚΑΘΟΛΙΚΕΣ ΜΕΤΑΒΛΗΤΕΣ ==================== */
init_variabledefs
    : init_variabledefs COMMA init_variabledef
    | init_variabledef
    ;

init_variabledef
    : variabledef initializer
    ;

/* ==================== ΕΝΤΟΛΕΣ ==================== */
decl_statements
    : declarations statements  { $$ = $2; }
    | declarations             { $$ = NULL; }
    | statements               { $$ = $1; }
    | /* empty */              { $$ = NULL; }
    ;

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

statements
    : statements statement
        { $$ = ast_append($1, $2); }
    | statement
        { $$ = $1; }
    ;

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
    | SEMI                  { $$ = NULL; }
    | error SEMI            { $$ = NULL; fprintf(stderr, "Error recovery at line %d\n", yylineno); }
    ;

expression_statement
    : general_expression SEMI
        { $$ = $1; }
    ;

if_statement
    : IF LPAREN general_expression RPAREN statement
      %prec LOWER_THAN_ELSE
        { $$ = ast_make_if($3, $5, NULL); }
    | IF LPAREN general_expression RPAREN statement ELSE statement
        { $$ = ast_make_if($3, $5, $7); }
    ;

while_statement
    : WHILE LPAREN general_expression RPAREN statement
        { $$ = ast_make_while($3, $5); }
    ;

for_statement
    : FOR LPAREN optexpr SEMI optexpr SEMI optexpr RPAREN statement
        { $$ = ast_make_for($3, $5, $7, $9); }
    ;

optexpr
    : general_expression  { $$ = $1; }
    | /* empty */         { $$ = NULL; }
    ;

return_statement
    : RETURN optexpr SEMI
        { $$ = ast_make_return($2); }
    ;

io_statement
    : CIN INP in_list SEMI
        { $$ = ast_make_cin($3); }
    | COUT OUT out_list SEMI
        { $$ = ast_make_cout($3); }
    ;

in_list
    : in_list INP in_item
        { $1->next = $3; $$ = $1; }
    | in_item
        { $$ = $1; }
    ;

in_item
    : variable  { $$ = $1; }
    ;

out_list
    : out_list OUT out_item
        { $1->next = $3; $$ = $1; }
    | out_item
        { $$ = $1; }
    ;

out_item
    : general_expression  { $$ = $1; }
    ;

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

main_header
    : INT MAIN LPAREN RPAREN
    ;

/* ==================== ΕΚΦΡΑΣΕΙΣ ==================== */
general_expression
    : general_expression COMMA general_expression
        { $$ = ast_make_binop(",", $1, $3); }
    | assignment
        { $$ = $1; }
    ;

assignment
    : variable ASSIGN assignment
        { $$ = ast_make_assign($1, $3); }
    | expression
        { $$ = $1; }
    ;

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
        { $$ = ast_make_unop("-", $2); }
    | SIZEOP expression
        { $$ = ast_make_unop("sizeof", $2); }
    | INCDEC variable %prec PREINCDEC
        { $$ = ast_make_unop("pre++", $2); }
    | variable INCDEC %prec POSTINCDEC
        { $$ = ast_make_unop("post++", $1); }
    | variable
        { $$ = $1; }
    | variable LPAREN expression_list RPAREN
        { $$ = ast_make_call($1->name, $3); }
    | LENGTH LPAREN general_expression RPAREN
        { $$ = ast_make_call("length", $3); }
    | constant
        { $$ = $1; }
    | LPAREN general_expression RPAREN
        { $$ = $2; }
    | LPAREN standard_type RPAREN
        { $$ = ast_make_iconst($2); }
    | listexpression
        { $$ = $1; }
    ;

variable
    : variable LBRACK general_expression RBRACK
        { $$ = ast_make_binop("[]", $1, $3); }
    | variable DOT ID
        { $$ = ast_make_binop(".", $1, ast_make_id($3, TYPE_UNKNOWN)); }
    | LISTFUNC LPAREN general_expression RPAREN
        { $$ = ast_make_call($1, $3); }
    | ID
        {
            Symbol *s = symtable_lookup(symtable, $1);
            SymType t = s ? s->type : TYPE_UNKNOWN;
            $$ = ast_make_id($1, t);
        }
    | THIS
        { $$ = ast_make_id("this", TYPE_UNKNOWN); }
    ;

expression_list
    : general_expression  { $$ = $1; }
    | /* empty */         { $$ = NULL; }
    ;

constant
    : CCONST  { $$ = ast_make_cconst($1); }
    | ICONST  { $$ = ast_make_iconst($1); }
    | FCONST  { $$ = ast_make_fconst($1); }
    | SCONST  { $$ = ast_make_sconst($1); }
    ;

listexpression
    : LBRACK expression_list RBRACK
        { $$ = $2; }
    ;

%%

void yyerror(const char *msg) {
    fprintf(stderr, "Syntax error at line %d: %s\n", yylineno, msg);
    error_count++;
    if (error_count >= 5) {
        fprintf(stderr, "Too many errors (5), aborting.\n");
        exit(1);
    }
}