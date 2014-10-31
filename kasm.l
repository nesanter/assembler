%option noyywrap yylineno

%{
    #include "kasm.h"
    #include "kasm.tab.h"
%}

%x DEFINE
%x IFDEF
%x IFNDEF
%x IGNORE

%%

 /* comments */
;.*$ 

 /* ---preprocessor--- */
"#DEFINE " { BEGIN(DEFINE); }
<DEFINE>[A-Z0-9_]+ { preproc_define(yytext); BEGIN(INITIAL); }
"#IFDEF " { BEGIN(IFDEF); }
<IFDEF>[A-Z0-9_]+ { if (preproc_isdefined(yytext)) BEGIN(INITIAL); else BEGIN(IGNORE); }
"#IFNDEF " { BEGIN(IFNDEF); }
<IFNDEF>[A-Z0-9_]+ { if (!preproc_isdefined(yytext)) BEGIN(INITIAL); else BEGIN(IGNORE); }

 /* ignore until matching #ENDIF */
<IGNORE>("#IFDEF "|"#IFNDEF ") { preproc_incdepth(); }
<IGNORE>"#ENDIF" { if (preproc_decdepth()) BEGIN(INITIAL); }
<IGNORE>. { printf("ignoring %s\n", yytext); }

<INITIAL>"#ENDIF"

 /* ---source & microcode--- */

 /* keywords */
"source:" { return ENTER_SOURCE; }
"microcode:" { return ENTER_MICROCODE; }

 /* numeric constants */
[0-9a-fA-F]+h { yylval.llu = strtoull(yytext, NULL, 16); return NUMERIC; }
[01]+b { yylval.llu = strtoull(yytext, NULL, 2); return NUMERIC; }
[0-9]+(d)? { yylval.llu = strtoull(yytext, NULL, 10); return NUMERIC; }

 /* textual identifiers */
[A-Z][A-Z0-9_]* { yylval.text = strdup(yytext); return IDENT_CAPS; }
[a-zA-Z][a-zA-Z0-9._]* { yylval.text = strdup(yytext); return IDENT; }

 /* register mark */
"%r" { return REGMARK; }

 /* option mark */
"\\opt" { return OPTION; }

 /* special symbols */
"%" { return PERCENT; }
"," { return COMMA; }
"=" { return EQ; }
":" { return COLON; }
"@" { return AT; }
"+" { return PLUS; }
"." { return DOT; }

"(" { return LP; }
")" { return RP; }
"{" { return LB; }
"}" { return RB; }
"[" { return LS; }
"]" { return RS; }

^\n
\n { return EOL; }

 /* whitespace */
[ \t]+

 /* catch-all */
. { fprintf(stderr, "Lexical error: unexpected symbol (line %d)\n", yylineno); }

%%

