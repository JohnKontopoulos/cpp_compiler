%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"

int line_num = 1;
char current_line[4096] = "";
char line_buf[4096] = "";

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
[Tt][Yy][Pp][Ee][Dd][Ee][Ff]         { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"TYPEDEF\n");    return TYPEDEF; }
[Cc][Hh][Aa][Rr]                     { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"CHAR\n");       return CHAR; }
[Ii][Nn][Tt]                         { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"INT\n");        return INT; }
[Ff][Ll][Oo][Aa][Tt]                 { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"FLOAT\n");      return FLOAT; }
[Ss][Tt][Rr][Ii][Nn][Gg]             { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"STRING\n");     return STRING; }
[Cc][Oo][Nn][Ss][Tt]                 { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"CONST\n");      return CONST; }
[Cc][Ll][Aa][Ss][Ss]                 { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"CLASS\n");      return CLASS; }
[Pp][Rr][Ii][Vv][Aa][Tt][Ee]         { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"PRIVATE\n");    return PRIVATE; }
[Pp][Rr][Oo][Tt][Ee][Cc][Tt][Ee][Dd] { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"PROTECTED\n"); return PROTECTED; }
[Pp][Uu][Bb][Ll][Ii][Cc]             { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"PUBLIC\n");     return PUBLIC; }
[Vv][Oo][Ii][Dd]                     { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"VOID\n");       return VOID; }
[Ss][Tt][Aa][Tt][Ii][Cc]             { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"STATIC\n");     return STATIC; }
[Uu][Nn][Ii][Oo][Nn]                 { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"UNION\n");      return UNION; }
[Ee][Nn][Uu][Mm]                     { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"ENUM\n");       return ENUM; }
[Ll][Ii][Ss][Tt]                     { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"LIST\n");       return LIST; }
[Cc][Oo][Nn][Tt][Ii][Nn][Uu][Ee]     { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"CONTINUE\n");  return CONTINUE; }
[Bb][Rr][Ee][Aa][Kk]                 { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"BREAK\n");      return BREAK; }
[Ii][Ff]                             { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"IF\n");         return IF; }
[Ee][Ll][Ss][Ee]                     { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"ELSE\n");       return ELSE; }
[Ww][Hh][Ii][Ll][Ee]                 { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"WHILE\n");      return WHILE; }
[Ff][Oo][Rr]                         { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"FOR\n");        return FOR; }
[Rr][Ee][Tt][Uu][Rr][Nn]             { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"RETURN\n");     return RETURN; }
[Ll][Ee][Nn][Gg][Tt][Hh]             { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"LENGTH\n");     return LENGTH; }
[Cc][Ii][Nn]                         { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"CIN\n");        return CIN; }
[Cc][Oo][Uu][Tt]                     { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"COUT\n");       return COUT; }
[Mm][Aa][Ii][Nn]                     { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"MAIN\n");       return MAIN; }
[Tt][Hh][Ii][Ss]                     { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"THIS\n");       return THIS; }
[Ss][Ii][Zz][Ee][Oo][Ff]             { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"SIZEOP\n");     return SIZEOP; }

 /* ==================== LISTFUNC ==================== */
[Cc]([Aa][Dd]*|[Dd]+)[Rr]   {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"LISTFUNC(%s)\n", yytext);
    yylval.sval = strdup(yytext);
    return LISTFUNC;
}

 /* ==================== ΤΕΛΕΣΤΕΣ ==================== */
"||"   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"OROP\n");   return OROP; }
"&&"   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"ANDOP\n");  return ANDOP; }
"=="   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"EQUOP\n");  return EQUOP; }
"!="   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"EQUOP\n");  return EQUOP; }
">="   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"RELOP\n");  return RELOP; }
"<="   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"RELOP\n");  return RELOP; }
">"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"RELOP\n");  return RELOP; }
"<"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"RELOP\n");  return RELOP; }
"+"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"ADDOP\n");  return ADDOP; }
"-"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"ADDOP\n");  return ADDOP; }
"*"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"MULOP\n");  return MULOP; }
"/"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"MULOP\n");  return MULOP; }
"%"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"MULOP\n");  return MULOP; }
"!"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"NOTOP\n");  return NOTOP; }
"++"   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"INCDEC\n"); return INCDEC; }
"--"   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"INCDEC\n"); return INCDEC; }
"::"   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"METH\n");   return METH; }
">>"   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"INP\n");    return INP; }
"<<"   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"OUT\n");    return OUT; }

 /* ==================== ΔΙΑΧΩΡΙΣΤΙΚΑ ==================== */
"("    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"LPAREN\n");  return LPAREN; }
")"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"RPAREN\n");  return RPAREN; }
";"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"SEMI\n");    return SEMI; }
"."    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"DOT\n");     return DOT; }
","    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"COMMA\n");   return COMMA; }
"="    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"ASSIGN\n");  return ASSIGN; }
":"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"COLON\n");   return COLON; }
"["    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"LBRACK\n");  return LBRACK; }
"]"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"RBRACK\n");  return RBRACK; }
"&"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"REFER\n");   return REFER; }
"{"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"LBRACE\n");  return LBRACE; }
"}"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"RBRACE\n");  return RBRACE; }

 /* ==================== ΑΚΕΡΑΙΕΣ ΣΤΑΘΕΡΕΣ ==================== */
0[Xx][1-9A-Fa-f][0-9A-Fa-f]*  {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"ICONST(%s)\n", yytext);
    yylval.ival = (int)strtol(yytext+2, NULL, 16);
    return ICONST;
}
0[Xx][A-Fa-f][0-9A-Fa-f]*     {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"ICONST(%s)\n", yytext);
    yylval.ival = (int)strtol(yytext+2, NULL, 16);
    return ICONST;
}
0[Oo][1-7][0-7]*               {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"ICONST(%s)\n", yytext);
    yylval.ival = (int)strtol(yytext+2, NULL, 8);
    return ICONST;
}
0[Bb][1][01]*                  {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"ICONST(%s)\n", yytext);
    yylval.ival = (int)strtol(yytext+2, NULL, 2);
    return ICONST;
}
0                              {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"ICONST(0)\n");
    yylval.ival = 0;
    return ICONST;
}
[1-9][0-9]*                    {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"ICONST(%s)\n", yytext);
    yylval.ival = atoi(yytext);
    return ICONST;
}

 /* ==================== ΠΡΑΓΜΑΤΙΚΕΣ ΣΤΑΘΕΡΕΣ ==================== */
[0-9]*"."[0-9]+([Ee][+-]?[0-9]+)?   {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"FCONST(%s)\n", yytext);
    yylval.fval = atof(yytext);
    return FCONST;
}
[1-9][0-9]*[Ee][+-]?[0-9]+          {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"FCONST(%s)\n", yytext);
    yylval.fval = atof(yytext);
    return FCONST;
}
0[Xx][0-9A-Fa-f]*"."[0-9A-Fa-f]+    {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"FCONST(%s)\n", yytext);
    yylval.fval = atof(yytext);
    return FCONST;
}

 /* ==================== ΧΑΡΑΚΤΗΡΕΣ (CCONST) ==================== */
'([ -~]|\\[nftrbv])'          {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"CCONST(%s)\n", yytext);
    yylval.cval = yytext[1];
    return CCONST;
}

 /* ==================== ΟΡΜΑΘΟΙ (SCONST) ==================== */
\"([^"\\\n]|\\.|\\\n)*\"       {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"SCONST(%s)\n", yytext);
    yylval.sval = strdup(yytext);
    return SCONST;
}

 /* ==================== ΑΝΑΓΝΩΡΙΣΤΙΚΑ (ID) ==================== */
"__"?{LETTER}({ALNUM}|("_"{ALNUM}))*  {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"ID(%s)\n", yytext);
    yylval.sval = strdup(yytext);
    return ID;
}

 /* ==================== ΑΛΛΑ ==================== */
[ \t\r]+    {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
}
\n          {
    strcpy(current_line, line_buf);
    memset(line_buf, 0, sizeof(line_buf));
    line_num++;
}

.   { lex_error(yytext); }

%%

void lex_error(char *msg) {
    /* Αποθήκευση τρέχουσας γραμμής */
    strcpy(current_line, line_buf);
    
    fprintf(stderr, "Lexical error at line %d: unexpected character '%s'\n",
            yylineno, msg);
    fprintf(stderr, ">>> Line %d: %s\n", yylineno, current_line);
    
    /* Προσπαθεί να συνεχίσει σβήνοντας τον λανθασμένο χαρακτήρα */
    /* Αν έχουμε πάρα πολλά errors, τερματίζουμε */
    static int lex_error_count = 0;
    lex_error_count++;
    if (lex_error_count >= 10) {
        fprintf(stderr, "Too many lexical errors, aborting.\n");
        exit(1);
    }
}