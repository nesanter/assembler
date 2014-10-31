#ifndef KASM_H
#define KASM_H

#include <stdint.h>

#define INST_OPCODE_BITS (11)

void warn();

typedef struct {
    char *ident;
    uint64_t bits;
} bitdef;

uint64_t create_bitdef(uint64_t bit);
uint64_t merge_bitdef(uint64_t def, uint64_t bit);
uint64_t merge_bitdef2(uint64_t def, uint64_t def2);
void register_bitdef(char *ident, uint64_t def);
uint64_t bitdef_lookup(char *ident);
void print_bitdefs();

typedef struct s_tag {
    char *ident;
    uint64_t value_numeric;
    char *value_ident;
    struct s_tag *next;
} tag;

tag* create_tag_empty(char *ident);
tag* create_tag_ident(char *ident, char *value);
tag* create_tag_numeric(char *ident, uint64_t value);
tag* append_tag(tag *a, tag *b);

typedef struct {
    char *ident;
    uint64_t value;
    uint64_t bits;
    uint64_t n_operands;
    uint64_t n_immediates;
    tag *tags;
    uint64_t n_tags;
} idef;

void register_idef(char *ident, uint64_t bits, tag *tags);
idef* idef_lookup(char *ident);
uint64_t get_definitions(idef ***table);
void print_idefs();
int has_tag(idef *i, char *ident, uint64_t *value_numeric, char **value_ident);

void set_microcode_bits(uint64_t n);
uint64_t get_microcode_bits();

void set_option(char *ident, uint64_t value);

uint64_t create_offset(uint64_t offset);

typedef struct {
    uint64_t base;
    uint64_t offset1;
    uint64_t offset2;
} operand;

operand* create_operand(uint64_t base, uint64_t offset1, uint64_t offset2);
void print_operand(FILE *f, operand *o);

typedef enum {
    NONE, SINGLE, DOUBLE, GLOBAL_LABEL, LOCAL_LABEL
} imm_type;

typedef struct s_inst {
    idef *def;
    uint64_t address;
    operand *oper1;
    operand *oper2;
    operand *oper3;
    imm_type type;
    uint64_t immediate;
    char *immediate_ident;
    struct s_inst *next;
    uint64_t real_address;
} inst;

typedef struct s_label {
    char *ident;
    uint64_t address;
    struct s_label *child;
} label;

int verify_inst(inst *i);
void register_inst(char *ident, operand *oper1, operand *oper2, operand *oper3, imm_type itype, uint64_t immediate, char *immediate_ident);
inst* get_instructions();
void print_instruction(FILE *f, inst *in, int real);

typedef enum {
    GLOBAL, LOCAL
} label_type;

void register_label(char *ident, label_type type);
label* label_lookup_global(char *ident);
label* label_lookup_local(label *parent, char *ident);

typedef enum {
    ABS, REL_AUTO, REL_IDENT
} section_type;

typedef struct {
    char *ident;
    uint64_t base;
    uint64_t size;
    inst **inst_table;
    uint64_t n_insts;
    label **label_table;
    uint64_t n_labels;
} section;

void register_rel_address(uint64_t address);
void register_abs_address(uint64_t address);

section* section_lookup(char *ident);
section* section_lookup_reverse(char *ident);

typedef struct {
    char *ident;
    uint64_t base;
} section_ident;

section_ident* create_section_ident(char *ident, section_type type, uint64_t base, char *base_ident);

void register_section(section_ident *sident);

typedef struct {
    uint64_t n_operands;
    uint64_t n_immediates;
    uint64_t label_allowed;
} idef_info;

idef_info *idef_get_info(idef *def);

void print_sections();
void print_sections_contents();

extern int yylineno;

void preproc_define(char *s);
int preproc_isdefined(char *s);
void preproc_incdepth();
int preproc_decdepth();

void yyerror(char *s, ...);

void print_bin(FILE *f, uint64_t n, uint64_t bits);
void print_hex(FILE *f, uint64_t n, uint64_t bits);

typedef enum {
    EF_MEMB, EF_MEMH
} emit_format;

void emit_microcode(FILE *f, int verbose, emit_format format);
void emit_instructions(FILE *f, int verbose, emit_format format, char *secname);
uint64_t encode_instruction(inst *i);
uint64_t encode_offset(operand *o);

#endif /* KASM_H */
