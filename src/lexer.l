%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"

int line_num = 1;
void lex_error(char *msg);
%}

%option noyywrap
%option yylineno

DIGIT       [0-9]
NONZERO     [1-9]
HEXDIGIT    [0-9A-Fa-f]
OCTDIGIT    [0-7]
BINDIGIT    [01]
LETTER      [a-zA-Z]
ALNUM       [a-zA-Z0-9]

%%

 /* ==================== ΣΧΟΛΙΑ ==================== */
"/*"([^*]|\*+[^*/])*\*+"/"   { /* block comment */ }
"//"[^\n]*                    { /* line comment */ }

 /* ==================== ΛΕΞΕΙΣ-ΚΛΕΙΔΙΑ ==================== */
[Tt][Yy][Pp][Ee][Dd][Ee][Ff]         { printf("TYPEDEF\n");    return TYPEDEF; }
[Cc][Hh][Aa][Rr]                     { printf("CHAR\n");       return CHAR; }
[Ii][Nn][Tt]                         { printf("INT\n");        return INT; }
[Ff][Ll][Oo][Aa][Tt]                 { printf("FLOAT\n");      return FLOAT; }
[Ss][Tt][Rr][Ii][Nn][Gg]             { printf("STRING\n");     return STRING; }
[Cc][Oo][Nn][Ss][Tt]                 { printf("CONST\n");      return CONST; }
[Cc][Ll][Aa][Ss][Ss]                 { printf("CLASS\n");      return CLASS; }
[Pp][Rr][Ii][Vv][Aa][Tt][Ee]         { printf("PRIVATE\n");    return PRIVATE; }
[Pp][Rr][Oo][Tt][Ee][Cc][Tt][Ee][Dd] { printf("PROTECTED\n"); return PROTECTED; }
[Pp][Uu][Bb][Ll][Ii][Cc]             { printf("PUBLIC\n");     return PUBLIC; }
[Vv][Oo][Ii][Dd]                     { printf("VOID\n");       return VOID; }
[Ss][Tt][Aa][Tt][Ii][Cc]             { printf("STATIC\n");     return STATIC; }
[Uu][Nn][Ii][Oo][Nn]                 { printf("UNION\n");      return UNION; }
[Ee][Nn][Uu][Mm]                     { printf("ENUM\n");       return ENUM; }
[Ll][Ii][Ss][Tt]                     { printf("LIST\n");       return LIST; }
[Cc][Oo][Nn][Tt][Ii][Nn][Uu][Ee]     { printf("CONTINUE\n");  return CONTINUE; }
[Bb][Rr][Ee][Aa][Kk]                 { printf("BREAK\n");      return BREAK; }
[Ii][Ff]                             { printf("IF\n");         return IF; }
[Ee][Ll][Ss][Ee]                     { printf("ELSE\n");       return ELSE; }
[Ww][Hh][Ii][Ll][Ee]                 { printf("WHILE\n");      return WHILE; }
[Ff][Oo][Rr]                         { printf("FOR\n");        return FOR; }
[Rr][Ee][Tt][Uu][Rr][Nn]             { printf("RETURN\n");     return RETURN; }
[Ll][Ee][Nn][Gg][Tt][Hh]             { printf("LENGTH\n");     return LENGTH; }
[Cc][Ii][Nn]                         { printf("CIN\n");        return CIN; }
[Cc][Oo][Uu][Tt]                     { printf("COUT\n");       return COUT; }
[Mm][Aa][Ii][Nn]                     { printf("MAIN\n");       return MAIN; }
[Tt][Hh][Ii][Ss]                     { printf("THIS\n");       return THIS; }
[Ss][Ii][Zz][Ee][Oo][Ff]             { printf("SIZEOP\n");     return SIZEOP; }

 /* ==================== LISTFUNC ==================== */
[Cc]([Aa][Dd]*|[Dd]+)[Rr]   {
    printf("LISTFUNC(%s)\n", yytext);
    yylval.sval = strdup(yytext);
    return LISTFUNC;
}

 /* ==================== ΤΕΛΕΣΤΕΣ ==================== */
"||"   { printf("OROP\n");   return OROP; }
"&&"   { printf("ANDOP\n");  return ANDOP; }
"=="   { printf("EQUOP\n");  return EQUOP; }
"!="   { printf("EQUOP\n");  return EQUOP; }
">="   { printf("RELOP\n");  return RELOP; }
"<="   { printf("RELOP\n");  return RELOP; }
">"    { printf("RELOP\n");  return RELOP; }
"<"    { printf("RELOP\n");  return RELOP; }
"+"    { printf("ADDOP\n");  return ADDOP; }
"-"    { printf("ADDOP\n");  return ADDOP; }
"*"    { printf("MULOP\n");  return MULOP; }
"/"    { printf("MULOP\n");  return MULOP; }
"%"    { printf("MULOP\n");  return MULOP; }
"!"    { printf("NOTOP\n");  return NOTOP; }
"++"   { printf("INCDEC\n"); return INCDEC; }
"--"   { printf("INCDEC\n"); return INCDEC; }
"::"   { printf("METH\n");   return METH; }
">>"   { printf("INP\n");    return INP; }
"<<"   { printf("OUT\n");    return OUT; }

 /* ==================== ΔΙΑΧΩΡΙΣΤΙΚΑ ==================== */
"("    { printf("LPAREN\n");  return LPAREN; }
")"    { printf("RPAREN\n");  return RPAREN; }
";"    { printf("SEMI\n");    return SEMI; }
"."    { printf("DOT\n");     return DOT; }
","    { printf("COMMA\n");   return COMMA; }
"="    { printf("ASSIGN\n");  return ASSIGN; }
":"    { printf("COLON\n");   return COLON; }
"["    { printf("LBRACK\n");  return LBRACK; }
"]"    { printf("RBRACK\n");  return RBRACK; }
"&"    { printf("REFER\n");   return REFER; }
"{"    { printf("LBRACE\n");  return LBRACE; }
"}"    { printf("RBRACE\n");  return RBRACE; }

 /* ==================== ΑΚΕΡΑΙΕΣ ΣΤΑΘΕΡΕΣ ==================== */

 /* Δεκαεξαδικοί: 0X... */
0[Xx][1-9A-Fa-f][0-9A-Fa-f]*  {
    printf("ICONST(%s)\n", yytext);
    yylval.ival = (int)strtol(yytext+2, NULL, 16);
    return ICONST;
}
0[Xx][A-Fa-f][0-9A-Fa-f]*     {
    printf("ICONST(%s)\n", yytext);
    yylval.ival = (int)strtol(yytext+2, NULL, 16);
    return ICONST;
}

 /* Οκταδικοί: 0O... */
0[Oo][1-7][0-7]*               {
    printf("ICONST(%s)\n", yytext);
    yylval.ival = (int)strtol(yytext+2, NULL, 8);
    return ICONST;
}

 /* Δυαδικοί: 0B... */
0[Bb][1][01]*                  {
    printf("ICONST(%s)\n", yytext);
    yylval.ival = (int)strtol(yytext+2, NULL, 2);
    return ICONST;
}

 /* Δεκαδικοί */
0                              {
    printf("ICONST(0)\n");
    yylval.ival = 0;
    return ICONST;
}
[1-9][0-9]*                    {
    printf("ICONST(%s)\n", yytext);
    yylval.ival = atoi(yytext);
    return ICONST;
}

 /* ==================== ΠΡΑΓΜΑΤΙΚΕΣ ΣΤΑΘΕΡΕΣ ==================== */
[0-9]*"."[0-9]+([Ee][+-]?[0-9]+)?   {
    printf("FCONST(%s)\n", yytext);
    yylval.fval = atof(yytext);
    return FCONST;
}
[1-9][0-9]*[Ee][+-]?[0-9]+          {
    printf("FCONST(%s)\n", yytext);
    yylval.fval = atof(yytext);
    return FCONST;
}
0[Xx][0-9A-Fa-f]*"."[0-9A-Fa-f]+    {
    printf("FCONST(%s)\n", yytext);
    yylval.fval = atof(yytext);
    return FCONST;
}

 /* ==================== ΧΑΡΑΚΤΗΡΕΣ (CCONST) ==================== */
'([ -~]|\\[nftrbv])'          {
    printf("CCONST(%s)\n", yytext);
    yylval.cval = yytext[1];
    return CCONST;
}

 /* ==================== ΟΡΜΑΘΟΙ (SCONST) ==================== */
\"([^"\\\n]|\\.|\\\n)*\"       {
    printf("SCONST(%s)\n", yytext);
    yylval.sval = strdup(yytext);
    return SCONST;
}

 /* ==================== ΑΝΑΓΝΩΡΙΣΤΙΚΑ (ID) ==================== */
"__"?{LETTER}({ALNUM}|("_"{ALNUM}))*  {
    printf("ID(%s)\n", yytext);
    yylval.sval = strdup(yytext);
    return ID;
}

 /* ==================== ΑΛΛΑ ==================== */
[ \t\r]+    { /* αγνοούμε whitespace */ }
\n          { line_num++; }

.   { lex_error(yytext); }

%%

void lex_error(char *msg) {
    fprintf(stderr, "Lexical error at line %d: unexpected character '%s'\n",
            yylineno, msg);
}