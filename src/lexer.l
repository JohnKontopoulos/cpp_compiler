/*
 * lexer.l
 * Λεκτικός Αναλυτής για τη γλώσσα CPP
 *
 * Χρησιμοποιεί το εργαλείο flex για την αυτόματη παραγωγή
 * του λεκτικού αναλυτή από τους κανόνες αναγνώρισης.
 *
 * Χαρακτηριστικά:
 * - Case-insensitive αναγνώριση λέξεων-κλειδιών (εκτός CCONST/SCONST)
 * - Υποστήριξη ακεραίων σε δεκαδική, δεκαεξαδική (0X), οκταδική (0O),
 *   και δυαδική (0B) μορφή
 * - Χειρισμός λεκτικών σφαλμάτων με εκτύπωση γραμμής και αριθμού
 * - Αποθήκευση τρέχουσας γραμμής για μηνύματα σφαλμάτων
 * - Όλα τα debug/info μηνύματα εκτυπώνονται στο stderr
 */
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"

/* Αριθμός τρέχουσας γραμμής (χρησιμοποιείται για μηνύματα σφαλμάτων) */
int line_num = 1;

/* Η τρέχουσα γραμμή που έχει ολοκληρωθεί (για μηνύματα σφαλμάτων) */
char current_line[4096] = "";

/* Buffer συλλογής χαρακτήρων της τρέχουσας γραμμής */
char line_buf[4096] = "";

void lex_error(char *msg);
%}

%option noyywrap
%option yylineno

 /* ==================== ΟΡΙΣΜΟΙ ΧΑΡΑΚΤΗΡΩΝ ==================== */
DIGIT       [0-9]
NONZERO     [1-9]
HEXDIGIT    [0-9A-Fa-f]
OCTDIGIT    [0-7]
BINDIGIT    [01]
LETTER      [a-zA-Z]
ALNUM       [a-zA-Z0-9]

%%

 /* ==================== ΣΧΟΛΙΑ ==================== */
 /* Block comments: αγνοούνται */
"/*"([^*]|\*+[^*/])*\*+"/"   { /* block comment */ }
 /* Line comments: αγνοούνται */
"//"[^\n]*                    { /* line comment */ }

 /* ==================== ΛΕΞΕΙΣ-ΚΛΕΙΔΙΑ ==================== */
 /* Αναγνωρίζονται case-insensitive */
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
 /* Συναρτήσεις λίστας: CAR, CDR, CADR, CADDR κλπ */
[Cc]([Aa][Dd]*|[Dd]+)[Rr]   {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"LISTFUNC(%s)\n", yytext);
    yylval.sval = strdup(yytext);
    return LISTFUNC;
}

 /* ==================== ΤΕΛΕΣΤΕΣ ==================== */
 /* Λογικοί */
"||"   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"OROP\n");   return OROP; }
"&&"   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"ANDOP\n");  return ANDOP; }
 /* Ισότητας */
"=="   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"EQUOP\n");  return EQUOP; }
"!="   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"EQUOP\n");  return EQUOP; }
 /* Σύγκρισης */
">="   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"RELOP\n");  return RELOP; }
"<="   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"RELOP\n");  return RELOP; }
">"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"RELOP\n");  return RELOP; }
"<"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"RELOP\n");  return RELOP; }
 /* Αριθμητικοί */
"+"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"ADDOP\n");  return ADDOP; }
"-"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"ADDOP\n");  return ADDOP; }
"*"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"MULOP\n");  return MULOP; }
"/"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"MULOP\n");  return MULOP; }
"%"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"MULOP\n");  return MULOP; }
 /* Μοναδιαίοι */
"!"    { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"NOTOP\n");  return NOTOP; }
"++"   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"INCDEC\n"); return INCDEC; }
"--"   { strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1); fprintf(stderr,"INCDEC\n"); return INCDEC; }
 /* Είσοδος/Έξοδος και μέθοδοι */
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
 /* Δεκαεξαδικές: 0X ακολουθούμενο από hex ψηφία */
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
 /* Οκταδικές: 0O ακολουθούμενο από oct ψηφία */
0[Oo][1-7][0-7]*               {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"ICONST(%s)\n", yytext);
    yylval.ival = (int)strtol(yytext+2, NULL, 8);
    return ICONST;
}
 /* Δυαδικές: 0B ακολουθούμενο από bin ψηφία */
0[Bb][1][01]*                  {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"ICONST(%s)\n", yytext);
    yylval.ival = (int)strtol(yytext+2, NULL, 2);
    return ICONST;
}
 /* Μηδέν */
0                              {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"ICONST(0)\n");
    yylval.ival = 0;
    return ICONST;
}
 /* Δεκαδικές: αρχίζουν από 1-9 */
[1-9][0-9]*                    {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"ICONST(%s)\n", yytext);
    yylval.ival = atoi(yytext);
    return ICONST;
}

 /* ==================== ΠΡΑΓΜΑΤΙΚΕΣ ΣΤΑΘΕΡΕΣ ==================== */
 /* Δεκαδικές με προαιρετικό εκθέτη */
[0-9]*"."[0-9]+([Ee][+-]?[0-9]+)?   {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"FCONST(%s)\n", yytext);
    yylval.fval = atof(yytext);
    return FCONST;
}
 /* Με εκθέτη χωρίς τελεία */
[1-9][0-9]*[Ee][+-]?[0-9]+          {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"FCONST(%s)\n", yytext);
    yylval.fval = atof(yytext);
    return FCONST;
}
 /* Δεκαεξαδικές πραγματικές */
0[Xx][0-9A-Fa-f]*"."[0-9A-Fa-f]+    {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"FCONST(%s)\n", yytext);
    yylval.fval = atof(yytext);
    return FCONST;
}

 /* ==================== ΧΑΡΑΚΤΗΡΕΣ (CCONST) ==================== */
 /* Χαρακτήρας σε μονά εισαγωγικά - case-sensitive */
'([ -~]|\\[nftrbv])'          {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"CCONST(%s)\n", yytext);
    yylval.cval = yytext[1];
    return CCONST;
}

 /* ==================== ΟΡΜΑΘΟΙ (SCONST) ==================== */
 /* Συμβολοσειρά σε διπλά εισαγωγικά - case-sensitive */
\"([^"\\\n]|\\.|\\\n)*\"       {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"SCONST(%s)\n", yytext);
    yylval.sval = strdup(yytext);
    return SCONST;
}

 /* ==================== ΑΝΑΓΝΩΡΙΣΤΙΚΑ (ID) ==================== */
 /* Μορφή: [__]letter(alnum|_alnum)* */
"__"?{LETTER}({ALNUM}|("_"{ALNUM}))*  {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
    fprintf(stderr,"ID(%s)\n", yytext);
    yylval.sval = strdup(yytext);
    return ID;
}

 /* ==================== WHITESPACE & NEWLINE ==================== */
 /* Κενά και tab: αγνοούνται αλλά συλλέγονται */
[ \t\r]+    {
    strncat(line_buf, yytext, sizeof(line_buf)-strlen(line_buf)-1);
}
 /* Νέα γραμμή: αποθήκευση γραμμής και μηδενισμός buffer */
\n          {
    strcpy(current_line, line_buf);
    memset(line_buf, 0, sizeof(line_buf));
    line_num++;
}

 /* ==================== ΛΕΚΤΙΚΑ ΣΦΑΛΜΑΤΑ ==================== */
.   { lex_error(yytext); }

%%

/*
 * lex_error - Χειρισμός λεκτικών σφαλμάτων
 * Εκτυπώνει μήνυμα με αριθμό γραμμής και περιεχόμενο γραμμής.
 * Συνεχίζει σβήνοντας τον λανθασμένο χαρακτήρα.
 * Τερματίζει μετά από 10 σφάλματα.
 */
void lex_error(char *msg) {
    strcpy(current_line, line_buf);
    fprintf(stderr, "Lexical error at line %d: unexpected character '%s'\n",
            yylineno, msg);
    fprintf(stderr, ">>> Line %d: %s\n", yylineno, current_line);
    static int lex_error_count = 0;
    lex_error_count++;
    if (lex_error_count >= 10) {
        fprintf(stderr, "Too many lexical errors, aborting.\n");
        exit(1);
    }
}