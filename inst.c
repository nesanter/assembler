#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "kasm.h"
#include "kasm.tab.h"

#define MAX_BASE 15LU
#define MAX_OFFSET 1LU

uint64_t current_address = 0;

inst **inst_table = NULL;
uint64_t n_insts = 0;

label **label_table = NULL;
uint64_t n_labels = 0;

section **section_table = NULL;
uint64_t n_sections = 0;

uint64_t create_offset(uint64_t offset) {
    if (offset > MAX_OFFSET) {
        fprintf(stderr, "Warning: offset %lu exceeds maximum of %lu, capping (line %d)\n", offset, MAX_OFFSET, yylineno);
        warn();

        return MAX_OFFSET + 1;
    }

    return offset + 1;
}

operand* create_operand(uint64_t base, uint64_t offset1, uint64_t offset2) {
    operand *o = malloc(sizeof(*o));

    if (base > MAX_BASE) {
        fprintf(stderr, "Warning: base register number %lu exceeds maximum of %lu, ignoring (line %d)\n", base, MAX_BASE, yylineno);
        warn();
        free(o);
        return NULL;
    }

    o->base = base;
    o->offset1 = offset1;
    o->offset2 = offset2;

    return o;
}

void register_inst(char *ident, operand *oper1, operand *oper2, operand *oper3, imm_type itype, uint64_t immediate, char *immediate_ident) {
    inst *i = malloc(sizeof(*i));

    i->def = idef_lookup(ident);

    if (!i->def) {
        fprintf(stderr, "Warning: no definition found for instruction %s, ignoring (line %d)\n", ident, yylineno);
        warn();
        free(i);
        return;
    }

    i->oper1 = oper1;
    i->oper2 = oper2;
    i->oper3 = oper3;

    i->type = itype;
    i->immediate = immediate;
    if (immediate_ident)
        i->immediate_ident = strdup(immediate_ident);
    else
        i->immediate_ident = NULL;

    i->address = current_address++;

    if (verify_inst(i)) {
        free(i);
        return;
    }

    if (inst_table) {
        inst_table = realloc(inst_table, sizeof(*inst_table) * (n_insts + 1));
    } else {
        inst_table = malloc(sizeof(*inst_table));
    }

    inst_table[n_insts++] = i;
}

void register_label(char *ident, label_type type) {
    label *l = malloc(sizeof(*l));

    l->ident = strdup(ident);
    l->address = current_address;
    l->child = NULL;

    if (type == GLOBAL) {
        if (label_table) {
            label_table = realloc(label_table, sizeof(*label_table) * (n_labels + 1));
        } else {
            label_table = malloc(sizeof(*label_table));
        }

        label_table[n_labels++] = l;
    } else if (type == LOCAL) {
        if (n_labels == 0) {
            fprintf(stderr, "Warning: local label %s without parent, ignoring (line %d)\n", ident, yylineno);
            warn();
            free(l);
        } else {
            label *tmp = label_table[n_labels - 1];

            while (tmp->child)
                tmp = tmp->child;

            tmp->child = l;
        }
    }
}

section_ident* create_section_ident(char *ident, section_type type, uint64_t base, char *base_ident) {
    section_ident *s = malloc(sizeof(*s));

    if (ident) {
        s->ident = strdup(ident);
    } else {
        char name[128];
        snprintf(name, 128, "*auto-%lu", n_sections);
        s->ident = strdup(name);
    }

    if (type == REL_IDENT) {
        section *base_section = section_lookup_reverse(base_ident);

        if (!base_section) {
            fprintf(stderr, "Warning: section %s not found prior to use as relative base, assuming 0 (line %d)\n", base_ident, yylineno);
            warn();
            s->base = 0;
        } else {
            s->base = base_section->base + base_section->size;
        }
    } else if (type == REL_AUTO) {
        if (section_table) {
            s->base = section_table[n_sections-1]->base + section_table[n_sections-1]->size;
        } else {
            fprintf(stderr, "Warning: section %s specified as auto without prior section to use as base (line %d)\n", ident, yylineno);
            warn();
            s->base = 0;
        }
    } else if (type == ABS) {
        s->base = base;
    }

    return s;
}

void register_section(section_ident *sident) {
    section *s = malloc(sizeof(*s));

    s->ident = sident->ident;
    s->base = sident->base;
  
    s->inst_table = inst_table;
    s->n_insts = n_insts;
    s->size = current_address;
    s->label_table = label_table;
    s->n_labels = n_labels;

    uint64_t j = 0;
    label* l = NULL;
    uint64_t addr = 0;
    for (uint64_t i = 0; i < n_insts; i++) {
        while (j < n_labels && label_table[j]->address <= addr) {
            l = label_table[j++];
        }
        inst *in = inst_table[i];

        if (in->type == GLOBAL_LABEL) {
            label *tmp = label_lookup_global(in->immediate_ident);

            if (tmp) {
                in->immediate = tmp->address;
            } else {
                fprintf(stderr, "Warning: no label %s in section %s, treating as 0 (line %d)\n", in->immediate_ident, s->ident, yylineno);
                warn();
                in->immediate = 0;
            }
        } else if (in->type == LOCAL_LABEL) {
            if (l) {
                label *tmp = label_lookup_local(l, in->immediate_ident);

                if (tmp) {
                    in->immediate = tmp->address;
                } else {
                    fprintf(stderr, "Warning: no local label %s in section %s, treating as 0 (line %d)\n", in->immediate_ident, s->ident, yylineno);
                    warn();
                    in->immediate = 0;
                }
            } else {
                fprintf(stderr, "Warning: local label without parent [you should never see this error] (line %d)\n", yylineno);
                warn();
                in->immediate = 0;
            }
        }

        addr = in->address;
    }

    inst_table = NULL;
    n_insts = 0;
    label_table = NULL;
    n_labels = 0;

    current_address = 0;

    if (section_table) {
        section_table = realloc(section_table, sizeof(*section_table) * (n_sections + 1));
    } else {
        section_table = malloc(sizeof(*section_table));
    }

    section_table[n_sections++] = s;
}

section* section_lookup(char *ident) {
    for (uint64_t i = 0; i < n_sections; i++) {
        if (strcmp(ident, section_table[i]->ident) == 0) {
            return section_table[i];
        }
    }

    return NULL;
}

section* section_lookup_reverse(char *ident) {
    for (uint64_t i = n_sections; i > 0; i--) {
        if (strcmp(ident, section_table[i-1]->ident) == 0) {
            return section_table[i-1];
        }
    }

    return NULL;
}

void register_rel_address(uint64_t address) {
    current_address += address;
}
void register_abs_address(uint64_t address) {
    if (address < current_address) {
        fprintf(stderr, "Warning: absolute address %lu less than current address %lu, ignoring (line %d)\n", address, current_address, yylineno);
    } else {
        current_address = address;
    }
}

void print_sections() {
    for (uint64_t i = 0; i < n_sections; i++) {
        printf("section %s:\n", section_table[i]->ident);
        printf("  base = %lu\n", section_table[i]->base);
        printf("  size = %lu\n", section_table[i]->size);
        printf("  n_insts = %lu\n", section_table[i]->n_insts);
        printf("  n_labels = %lu\n", section_table[i]->n_labels);
    }
}

void print_sections_contents() {
    for (uint64_t i = 0; i < n_sections; i++) {
        printf("contents of section %s:\n", section_table[i]->ident);

        uint64_t l = 0;
        uint64_t addr = 0;
        for (uint64_t j = 0; j < section_table[i]->n_insts; j++) {
            inst *in = section_table[i]->inst_table[j];

            if (in->address > addr + 1) {
                printf("  (skip %lu)\n", in->address - addr - 1);
            }
            
            addr = in->address;

            while (l < section_table[i]->n_labels && section_table[i]->label_table[l]->address <= addr) {
                printf("  [%s] @ %lu\n", section_table[i]->label_table[l]->ident, addr);
                l++;
            }

            print_instruction(stdout, in, 0);

            printf("\n");
        }
    }
}

void print_instruction(FILE *f, inst *in, int real) {
    fprintf(f, "  %s ", in->def->ident);

    switch (in->type) {
        case NONE:
            print_operand(f, in->oper1);
            fprintf(f, ", ");
            print_operand(f, in->oper2);
            fprintf(f, ", ");
            print_operand(f, in->oper3);
            break;
        case SINGLE:
            print_operand(f, in->oper1);
            fprintf(f, ", ");
            print_operand(f, in->oper2);
            fprintf(f, ", %lu", in->immediate);
            break;
        case DOUBLE:
        case GLOBAL_LABEL:
        case LOCAL_LABEL:
            print_operand(f, in->oper1);
            fprintf(f, ", %lu", in->immediate);
            break;
        default:
            fprintf(f, "oops");
    }

    if (real)
        fprintf(f, " @ %lu", in->real_address);
    else
        fprintf(f, " @ %lu", in->address);
}


void print_operand(FILE *f, operand *o) {
    if (!o) {
        fprintf(f, "(null)");
        return;
    }
    fprintf(f, "r%lu", o->base);
    if (o->offset1) {
        fprintf(f, "[%lu]", o->offset1 - 1);
        if (o->offset2) {
            fprintf(f, "[%lu]", o->offset2 - 1);
        }
    }
}

label* label_lookup_global(char *ident) {
    for (uint64_t i = 0; i < n_labels; i++) {
        if (strcmp(ident, label_table[i]->ident) == 0) {
            return label_table[i];
        }
    }

    return NULL;
}

label* label_lookup_local(label *parent, char *ident) {
    while (parent->child) {
        if (strcmp(ident, parent->child->ident) == 0) {
            return parent->child;
        }

        parent = parent->child;
    }

    return NULL;
}

int verify_inst(inst *i) {
    uint64_t n_operands = 0;
    uint64_t n_immediates;

    if (i->oper1)
        n_operands++;
    if (i->oper2)
        n_operands++;
    if (i->oper3)
        n_operands++;

    switch (i->type) {
        case NONE:
            n_immediates = 0;
            break;
        case SINGLE:
            n_immediates = 1;
            break;
        case DOUBLE:
        case GLOBAL_LABEL:
        case LOCAL_LABEL:
            n_immediates = 2;
            break;
    }

    idef_info *info = idef_get_info(i->def);

    if (n_operands != info->n_operands) {
        fprintf(stderr, "Warning: incorrect number of operands for instruction (line %d)\n", yylineno);
        warn();
        return 1;
    }

    if (n_immediates != info->n_immediates) {
        fprintf(stderr, "Warning: incorrect immediate type for instruction (line %d)\n", yylineno);
        warn();
        return 1;
    }

    if (!info->label_allowed && (i->type == GLOBAL_LABEL || i->type == LOCAL_LABEL)) {
        fprintf(stderr, "Warning: label not allowed as immediate for instruction (line %d)\n", yylineno);
        warn();
        return 1;
    }

    return 0;
}

inst* get_instructions(char *secname) {
    inst *tail = NULL;
    uint64_t addr = 0, newaddr;

    for (uint64_t i = 0; i < n_sections; i++) {
        if (secname && (strcmp(section_table[i]->ident, secname) != 0))
            continue;

        for (uint64_t j = 0; j < section_table[i]->n_insts; j++) {
            newaddr = section_table[i]->inst_table[j]->address + section_table[i]->base;
            if (newaddr > addr || !tail) {
                if (tail) {
                    section_table[i]->inst_table[j]->next = tail;
                } else {
                    section_table[i]->inst_table[j]->next = NULL;
                }
                tail = section_table[i]->inst_table[j];

                tail->real_address = newaddr;
                addr = newaddr;
            } else if (newaddr == addr) {
                fprintf(stderr, "Warning: conflicting instructions at address %lu\n", newaddr);
                warn();
            } else {
                inst *tmp = tail;
                while (tmp->next) {
                    if (tmp->next->address == newaddr) {
                        fprintf(stderr, "Warning: conflicting instructions at address %lu\n", newaddr);
                        warn();
                        break;
                    } else if (tmp->next->address < newaddr) {
                        section_table[i]->inst_table[j]->next = tmp->next;
                        tmp->next = section_table[i]->inst_table[j];
                        tmp->next->real_address = newaddr;
                        break;
                    }
                    tmp = tmp->next;
                }
                if (!tmp->next) {
                    tmp->next = section_table[i]->inst_table[j];
                    tmp->next->real_address = newaddr;
                    tmp->next->next = NULL;
                }
            }
        }
    }

    //reverse
    
    inst *next = NULL, *prev = NULL;

    while (tail) {
        next = tail->next;
        tail->next = prev;
        prev = tail;
        tail = next;
    }

    return prev;
}
