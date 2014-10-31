%{
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include "kasm.h"
%}

%union {
    uint64_t llu;
    char *text;
    tag *t;
    operand *o;
    section_ident *sident;
}

%token <llu> NUMERIC
%token <text> IDENT IDENT_CAPS
%token ENTER_MICROCODE ENTER_SOURCE
%token COMMA LP RP LB RB LS RS EQ EOL PERCENT COLON AT PLUS DOT REGMARK OPTION

%type <text> any_ident
%type <llu> bitdef_bits idef_bits idef_bits_list offset base
%type <t> idef_tag_list idef_tag_term idef_tag
%type <o> operand
%type <sident> src_section_ident

%%

/* top level structure */

section:
%empty
| section uc_section_header
| section src_section_header
| error EOL { fprintf(stderr, "Syntax error in toplevel (line %d)\n", yylineno); yyerrok; }
;

eols:
%empty
| eols EOL
;

any_ident:
IDENT
| IDENT_CAPS
;

/* microcode definitions */

uc_section_header:
ENTER_MICROCODE eols uc_section
;

uc_section:
uc_toplevel_exp
| uc_section uc_toplevel_exp
;

uc_toplevel_exp:
bitdef_exp EOL
| option_exp EOL
| idef_exp EOL
| error EOL { fprintf(stderr, "Syntax error in microcode section (line %d)\n", yylineno); yyerrok; }
;

bitdef_exp:
PERCENT IDENT_CAPS bitdef_bits { register_bitdef($2, $3); }
;

option_exp:
OPTION any_ident NUMERIC { set_option($2, $3); }
;

idef_exp:
any_ident idef_bits { register_idef($1, $2, NULL); }
| any_ident idef_bits idef_tag { register_idef($1, $2, $3); }
;

bitdef_bits:
NUMERIC { $$ = create_bitdef($1); }
| bitdef_bits COMMA NUMERIC { $$ = merge_bitdef($1, $3); }
;

idef_bits:
LP idef_bits_list RP { $$ = $2; }
;

idef_bits_list:
%empty { $$ = 0; }
| NUMERIC { $$ = create_bitdef($1); }
| IDENT_CAPS { $$ = bitdef_lookup($1); }
| idef_bits_list COMMA NUMERIC { $$ = merge_bitdef($1, $3); }
| idef_bits_list COMMA IDENT_CAPS { $$ = merge_bitdef2($1, bitdef_lookup($3)); }
;

idef_tag:
LB RB { $$ = NULL; }
| LB idef_tag_list RB { $$ = $2; }
;

idef_tag_list:
idef_tag_term
| idef_tag_list COMMA idef_tag_term { $$ = append_tag($1, $3); }
;

idef_tag_term:
any_ident { $$ = create_tag_empty($1); }
| any_ident EQ any_ident { $$ = create_tag_ident($1, $3); }
| any_ident EQ NUMERIC { $$ = create_tag_numeric($1, $3); }
;


/* source code */

src_section_header:
ENTER_SOURCE src_section_ident eols src_section { register_section($2); }
;

src_section_ident:
%empty { $$ = create_section_ident(NULL, REL_AUTO, 0, NULL); }
| LB any_ident RB { $$ = create_section_ident($2, REL_AUTO, 0, NULL); }
| LB NUMERIC RB { $$ = create_section_ident(NULL, ABS, $2, NULL); }
| LB any_ident COMMA NUMERIC RB { $$ = create_section_ident($2, ABS, $4, NULL); }
| LB any_ident COMMA PLUS any_ident RB { $$ = create_section_ident($2, REL_IDENT, 0, $5); }
 

src_section:
src_toplevel_exp
| src_section src_toplevel_exp
;

src_toplevel_exp:
inst EOL
| label EOL
| address EOL
| error EOL { fprintf(stderr, "Syntax error in source section (line %d)\n", yylineno); yyerrok; }
;

inst:
any_ident { register_inst($1, NULL, NULL, NULL, NONE, 0, NULL); }
| any_ident operand { register_inst($1, $2, NULL, NULL, NONE, 0, NULL); }
| any_ident operand COMMA operand { register_inst($1, $2, $4, NULL, NONE, 0, NULL); }
| any_ident operand COMMA NUMERIC { register_inst($1, $2, NULL, NULL, DOUBLE, $4, NULL); }
| any_ident operand COMMA COLON any_ident { register_inst($1, $2, NULL, NULL, GLOBAL_LABEL, 0, $5); }
| any_ident operand COMMA COLON DOT any_ident { register_inst($1, $2, NULL, NULL, LOCAL_LABEL, 0, $6); }
| any_ident operand COMMA operand COMMA operand { register_inst($1, $2, $4, $6, NONE, 0, NULL); }
| any_ident operand COMMA operand COMMA NUMERIC { register_inst($1, $2, $4, NULL, SINGLE, $6, NULL); }
;

operand:
base { $$ = create_operand($1, 0, 0); }
| base offset { $$ = create_operand($1, $2, 0); }
| base offset offset { $$ = create_operand($1, $2, $3); }
;

base:
REGMARK NUMERIC { $$ = $2; }
;

offset:
LS NUMERIC RS { $$ = create_offset($2); }
;

label:
any_ident COLON { register_label($1, GLOBAL); }
| DOT any_ident COLON { register_label($2, LOCAL); }
;

address:
AT NUMERIC COLON { register_abs_address($2); }
| AT PLUS NUMERIC COLON { register_rel_address($3); }
;

%%
