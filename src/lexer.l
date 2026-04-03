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
[Tt][Yy][Pp][Ee][Dd][Ee][Ff]         { fprintf(stderr,"TYPEDEF\n");    return TYPEDEF; }
[Cc][Hh][Aa][Rr]                     { fprintf(stderr,"CHAR\n");       return CHAR; }
[Ii][Nn][Tt]                         { fprintf(stderr,"INT\n");        return INT; }
[Ff][Ll][Oo][Aa][Tt]                 { fprintf(stderr,"FLOAT\n");      return FLOAT; }
[Ss][Tt][Rr][Ii][Nn][Gg]             { fprintf(stderr,"STRING\n");     return STRING; }
[Cc][Oo][Nn][Ss][Tt]                 { fprintf(stderr,"CONST\n");      return CONST; }
[Cc][Ll][Aa][Ss][Ss]                 { fprintf(stderr,"CLASS\n");      return CLASS; }
[Pp][Rr][Ii][Vv][Aa][Tt][Ee]         { fprintf(stderr,"PRIVATE\n");    return PRIVATE; }
[Pp][Rr][Oo][Tt][Ee][Cc][Tt][Ee][Dd] { fprintf(stderr,"PROTECTED\n"); return PROTECTED; }
[Pp][Uu][Bb][Ll][Ii][Cc]             { fprintf(stderr,"PUBLIC\n");     return PUBLIC; }
[Vv][Oo][Ii][Dd]                     { fprintf(stderr,"VOID\n");       return VOID; }
[Ss][Tt][Aa][Tt][Ii][Cc]             { fprintf(stderr,"STATIC\n");     return STATIC; }
[Uu][Nn][Ii][Oo][Nn]                 { fprintf(stderr,"UNION\n");      return UNION; }
[Ee][Nn][Uu][Mm]                     { fprintf(stderr,"ENUM\n");       return ENUM; }
[Ll][Ii][Ss][Tt]                     { fprintf(stderr,"LIST\n");       return LIST; }
[Cc][Oo][Nn][Tt][Ii][Nn][Uu][Ee]     { fprintf(stderr,"CONTINUE\n");  return CONTINUE; }
[Bb][Rr][Ee][Aa][Kk]                 { fprintf(stderr,"BREAK\n");      return BREAK; }
[Ii][Ff]                             { fprintf(stderr,"IF\n");         return IF; }
[Ee][Ll][Ss][Ee]                     { fprintf(stderr,"ELSE\n");       return ELSE; }
[Ww][Hh][Ii][Ll][Ee]                 { fprintf(stderr,"WHILE\n");      return WHILE; }
[Ff][Oo][Rr]                         { fprintf(stderr,"FOR\n");        return FOR; }
[Rr][Ee][Tt][Uu][Rr][Nn]             { fprintf(stderr,"RETURN\n");     return RETURN; }
[Ll][Ee][Nn][Gg][Tt][Hh]             { fprintf(stderr,"LENGTH\n");     return LENGTH; }
[Cc][Ii][Nn]                         { fprintf(stderr,"CIN\n");        return CIN; }
[Cc][Oo][Uu][Tt]                     { fprintf(stderr,"COUT\n");       return COUT; }
[Mm][Aa][Ii][Nn]                     { fprintf(stderr,"MAIN\n");       return MAIN; }
[Tt][Hh][Ii][Ss]                     { fprintf(stderr,"THIS\n");       return THIS; }
[Ss][Ii][Zz][Ee][Oo][Ff]             { fprintf(stderr,"SIZEOP\n");     return SIZEOP; }

 /* ==================== LISTFUNC ==================== */
[Cc]([Aa][Dd]*|[Dd]+)[Rr]   {
    fprintf(stderr,"LISTFUNC(%s)\n", yytext);
    yylval.sval = strdup(yytext);
    return LISTFUNC;
}

 /* ==================== ΤΕΛΕΣΤΕΣ ==================== */
"||"   { fprintf(stderr,"OROP\n");   return OROP; }
"&&"   { fprintf(stderr,"ANDOP\n");  return ANDOP; }
"=="   { fprintf(stderr,"EQUOP\n");  return EQUOP; }
"!="   { fprintf(stderr,"EQUOP\n");  return EQUOP; }
">="   { fprintf(stderr,"RELOP\n");  return RELOP; }
"<="   { fprintf(stderr,"RELOP\n");  return RELOP; }
">"    { fprintf(stderr,"RELOP\n");  return RELOP; }
"<"    { fprintf(stderr,"RELOP\n");  return RELOP; }
"+"    { fprintf(stderr,"ADDOP\n");  return ADDOP; }
"-"    { fprintf(stderr,"ADDOP\n");  return ADDOP; }
"*"    { fprintf(stderr,"MULOP\n");  return MULOP; }
"/"    { fprintf(stderr,"MULOP\n");  return MULOP; }
"%"    { fprintf(stderr,"MULOP\n");  return MULOP; }
"!"    { fprintf(stderr,"NOTOP\n");  return NOTOP; }
"++"   { fprintf(stderr,"INCDEC\n"); return INCDEC; }
"--"   { fprintf(stderr,"INCDEC\n"); return INCDEC; }
"::"   { fprintf(stderr,"METH\n");   return METH; }
">>"   { fprintf(stderr,"INP\n");    return INP; }
"<<"   { fprintf(stderr,"OUT\n");    return OUT; }

 /* ==================== ΔΙΑΧΩΡΙΣΤΙΚΑ ==================== */
"("    { fprintf(stderr,"LPAREN\n");  return LPAREN; }
")"    { fprintf(stderr,"RPAREN\n");  return RPAREN; }
";"    { fprintf(stderr,"SEMI\n");    return SEMI; }
"."    { fprintf(stderr,"DOT\n");     return DOT; }
","    { fprintf(stderr,"COMMA\n");   return COMMA; }
"="    { fprintf(stderr,"ASSIGN\n");  return ASSIGN; }
":"    { fprintf(stderr,"COLON\n");   return COLON; }
"["    { fprintf(stderr,"LBRACK\n");  return LBRACK; }
"]"    { fprintf(stderr,"RBRACK\n");  return RBRACK; }
"&"    { fprintf(stderr,"REFER\n");   return REFER; }
"{"    { fprintf(stderr,"LBRACE\n");  return LBRACE; }
"}"    { fprintf(stderr,"RBRACE\n");  return RBRACE; }

 /* ==================== ΑΚΕΡΑΙΕΣ ΣΤΑΘΕΡΕΣ ==================== */
0[Xx][1-9A-Fa-f][0-9A-Fa-f]*  {
    fprintf(stderr,"ICONST(%s)\n", yytext);
    yylval.ival = (int)strtol(yytext+2, NULL, 16);
    return ICONST;
}
0[Xx][A-Fa-f][0-9A-Fa-f]*     {
    fprintf(stderr,"ICONST(%s)\n", yytext);
    yylval.ival = (int)strtol(yytext+2, NULL, 16);
    return ICONST;
}
0[Oo][1-7][0-7]*               {
    fprintf(stderr,"ICONST(%s)\n", yytext);
    yylval.ival = (int)strtol(yytext+2, NULL, 8);
    return ICONST;
}
0[Bb][1][01]*                  {
    fprintf(stderr,"ICONST(%s)\n", yytext);
    yylval.ival = (int)strtol(yytext+2, NULL, 2);
    return ICONST;
}
0                              {
    fprintf(stderr,"ICONST(0)\n");
    yylval.ival = 0;
    return ICONST;
}
[1-9][0-9]*                    {
    fprintf(stderr,"ICONST(%s)\n", yytext);
    yylval.ival = atoi(yytext);
    return ICONST;
}

 /* ==================== ΠΡΑΓΜΑΤΙΚΕΣ ΣΤΑΘΕΡΕΣ ==================== */
[0-9]*"."[0-9]+([Ee][+-]?[0-9]+)?   {
    fprintf(stderr,"FCONST(%s)\n", yytext);
    yylval.fval = atof(yytext);
    return FCONST;
}
[1-9][0-9]*[Ee][+-]?[0-9]+          {
    fprintf(stderr,"FCONST(%s)\n", yytext);
    yylval.fval = atof(yytext);
    return FCONST;
}
0[Xx][0-9A-Fa-f]*"."[0-9A-Fa-f]+    {
    fprintf(stderr,"FCONST(%s)\n", yytext);
    yylval.fval = atof(yytext);
    return FCONST;
}

 /* ==================== ΧΑΡΑΚΤΗΡΕΣ (CCONST) ==================== */
'([ -~]|\\[nftrbv])'          {
    fprintf(stderr,"CCONST(%s)\n", yytext);
    yylval.cval = yytext[1];
    return CCONST;
}

 /* ==================== ΟΡΜΑΘΟΙ (SCONST) ==================== */
\"([^"\\\n]|\\.|\\\n)*\"       {
    fprintf(stderr,"SCONST(%s)\n", yytext);
    yylval.sval = strdup(yytext);
    return SCONST;
}

 /* ==================== ΑΝΑΓΝΩΡΙΣΤΙΚΑ (ID) ==================== */
"__"?{LETTER}({ALNUM}|("_"{ALNUM}))*  {
    fprintf(stderr,"ID(%s)\n", yytext);
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