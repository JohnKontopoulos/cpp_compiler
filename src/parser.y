%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtable.h"

extern int yylex();
extern int yylineno;
extern char *yytext;

void yyerror(const char *msg);
int error_count = 0;

SymTable *symtable;

SymType current_type = TYPE_UNKNOWN;
int current_is_static = 0;
%}

%union {
    int   ival;
    float fval;
    char  cval;
    char  *sval;
}

%type <ival> standard_type typename

/* Tokens */
%token TYPEDEF CHAR INT FLOAT STRING CONST CLASS
%token PRIVATE PROTECTED PUBLIC VOID STATIC UNION ENUM LIST
%token CONTINUE BREAK IF ELSE WHILE FOR RETURN LENGTH
%token CIN COUT MAIN THIS SIZEOP
%token LISTFUNC
%token OROP ANDOP EQUOP RELOP ADDOP MULOP NOTOP INCDEC
%token LPAREN RPAREN SEMI DOT COMMA ASSIGN COLON
%token LBRACK RBRACK REFER LBRACE RBRACE METH INP OUT
%token <ival> ICONST
%token <fval> FCONST
%token <cval> CCONST
%token <sval> SCONST
%token <sval> ID

/* Προτεραιότητα τελεστών - από χαμηλότερη προς υψηλότερη */
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
        { printf("Program parsed successfully!\n"); }
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
    | global_var_declaration
    | func_declaration
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
    : ENUM ID enum_body SEMI
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
    : CLASS ID class_body SEMI
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
    : short_func_declaration
    ;

/* ==================== UNION ==================== */
union_declaration
    : UNION ID union_body SEMI
    ;

/* ==================== ΣΥΝΑΡΤΗΣΕΙΣ ==================== */
short_func_declaration
    : short_par_func_header SEMI
    | nopar_func_header SEMI
    ;

short_par_func_header
    : func_header_start LPAREN parameter_types RPAREN
    ;

func_header_start
    : typename listspec ID
    ;

parameter_types
    : parameter_types COMMA typename pass_list_dims
    | typename pass_list_dims
    ;

pass_list_dims
    : REFER
    | listspec dims
    ;

nopar_func_header
    : func_header_start LPAREN RPAREN
    ;

/* ==================== ΚΑΘΟΛΙΚΕΣ ΜΕΤΑΒΛΗΤΕΣ ==================== */
global_var_declaration
    : typename init_variabledefs SEMI
    ;

init_variabledefs
    : init_variabledefs COMMA init_variabledef
    | init_variabledef
    ;

init_variabledef
    : variabledef initializer
    ;

/* ==================== ΔΗΛΩΣΕΙΣ ΣΥΝΑΡΤΗΣΕΩΝ ==================== */
func_declaration
    : short_func_declaration
    | full_func_declaration
    ;

full_func_declaration
    : full_par_func_header LBRACE decl_statements RBRACE
    | nopar_class_func_header LBRACE decl_statements RBRACE
    | nopar_func_header LBRACE decl_statements RBRACE
    ;

full_par_func_header
    : class_func_header_start LPAREN parameter_list RPAREN
    | func_header_start LPAREN parameter_list RPAREN
    ;

class_func_header_start
    : typename listspec func_class ID
    ;

func_class
    : ID METH
    ;

parameter_list
    : parameter_list COMMA typename pass_variabledef
    | typename pass_variabledef
    ;

pass_variabledef
    : variabledef
    | REFER ID
    ;

nopar_class_func_header
    : class_func_header_start LPAREN RPAREN
    ;

/* ==================== ΕΝΤΟΛΕΣ ==================== */
decl_statements
    : declarations statements
    | declarations
    | statements
    | /* empty */
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
    | statement
    ;

statement
    : expression_statement
    | if_statement
    | while_statement
    | for_statement
    | return_statement
    | io_statement
    | comp_statement
    | CONTINUE SEMI
    | BREAK SEMI
    | SEMI
    ;

expression_statement
    : general_expression SEMI
    ;

if_statement
    : IF LPAREN general_expression RPAREN statement %prec LOWER_THAN_ELSE
    | IF LPAREN general_expression RPAREN statement ELSE statement
    ;

while_statement
    : WHILE LPAREN general_expression RPAREN statement
    ;

for_statement
    : FOR LPAREN optexpr SEMI optexpr SEMI optexpr RPAREN statement
    ;

optexpr
    : general_expression
    | /* empty */
    ;

return_statement
    : RETURN optexpr SEMI
    ;

io_statement
    : CIN INP in_list SEMI
    | COUT OUT out_list SEMI
    ;

in_list
    : in_list INP in_item
    | in_item
    ;

in_item
    : variable
    ;

out_list
    : out_list OUT out_item
    | out_item
    ;

out_item
    : general_expression
    ;

comp_statement
    : LBRACE decl_statements RBRACE
    ;

/* ==================== MAIN ==================== */
main_function
    : main_header LBRACE 
        { symtable_enter_scope(symtable); }
      decl_statements RBRACE
        { symtable_exit_scope(symtable); }
    ;

full_func_declaration
    : full_par_func_header LBRACE 
        { symtable_enter_scope(symtable); }
      decl_statements RBRACE
        { symtable_exit_scope(symtable); }
    | nopar_class_func_header LBRACE 
        { symtable_enter_scope(symtable); }
      decl_statements RBRACE
        { symtable_exit_scope(symtable); }
    | nopar_func_header LBRACE 
        { symtable_enter_scope(symtable); }
      decl_statements RBRACE
        { symtable_exit_scope(symtable); }
    ;

comp_statement
    : LBRACE 
        { symtable_enter_scope(symtable); }
      decl_statements RBRACE
        { symtable_exit_scope(symtable); }
    ;

main_header
    : INT MAIN LPAREN RPAREN
    ;

/* ==================== ΕΚΦΡΑΣΕΙΣ ==================== */
general_expression
    : general_expression COMMA general_expression
    | assignment
    ;

assignment
    : variable ASSIGN assignment
    | expression
    ;

expression
    : expression OROP expression
    | expression ANDOP expression
    | expression EQUOP expression
    | expression RELOP expression
    | expression ADDOP expression
    | expression MULOP expression
    | NOTOP expression
    | ADDOP expression %prec UMINUS
    | SIZEOP expression
    | INCDEC variable %prec PREINCDEC
    | variable INCDEC %prec POSTINCDEC
    | variable
    | variable LPAREN expression_list RPAREN
    | LENGTH LPAREN general_expression RPAREN
    | constant
    | LPAREN general_expression RPAREN
    | LPAREN standard_type RPAREN
    | listexpression
    ;

variable
    : variable LBRACK general_expression RBRACK
    | variable DOT ID
    | LISTFUNC LPAREN general_expression RPAREN
    | ID
    | THIS
    ;

expression_list
    : general_expression
    | /* empty */
    ;

constant
    : CCONST
    | ICONST
    | FCONST
    | SCONST
    ;

listexpression
    : LBRACK expression_list RBRACK
    ;

%%

void yyerror(const char *msg) {
    fprintf(stderr, "Syntax error at line %d: %s\n", yylineno, msg);
    error_count++;
    if (error_count >= 5) {
        fprintf(stderr, "Too many errors, aborting.\n");
        exit(1);
    }
}