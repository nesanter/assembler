#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "kasm.h"

idef **idef_table;
uint64_t n_idefs;

uint64_t idef_max_bits = 0;

tag* create_tag_empty(char *ident) {
    tag *t = malloc(sizeof(*t));

    t->ident = strdup(ident);
    t->value_numeric = 0;
    t->value_ident = NULL;
    t->next = NULL;

    return t;
}

tag* create_tag_ident(char *ident, char *value) {
    tag *t = malloc(sizeof(*t));

    t->ident = strdup(ident);
    t->value_numeric = 0;
    t->value_ident = strdup(value);
    t->next = NULL;

    return t;
}

tag* create_tag_numeric(char *ident, uint64_t value) {
    tag *t = malloc(sizeof(*t));

    t->ident = strdup(ident);
    t->value_numeric = value + 1;
    t->value_ident = NULL;
    t->next = NULL;

    return t;
}

tag* append_tag(tag *a, tag *b) {
    b->next = a;
    return b;
}

void register_idef(char *ident, uint64_t bits, tag *tags) {
    idef *i = malloc(sizeof(*i));

    i->ident = strdup(ident);
    i->bits = bits;
    if (bits >= (1LU << idef_max_bits)) {
        fprintf(stderr, "Warning: definition implicitly increases maximum bit width above %lu (line %d)\n", idef_max_bits, yylineno);
        warn();

        while (bits >= (1LU << idef_max_bits)) {
            idef_max_bits++;
        }
    }

    i->n_operands = 3;
    i->n_immediates = 3;
    i->tags = tags;

    /*
    tag *t = tags;

    
    while (t) {
        if (strcmp(t->ident, "op") == 0) {
            if (t->value_numeric == 0) {
                fprintf(stderr, "Warning: tag op without numeric key value, ignoring\n");
                warn();
            } else if (t->value_numeric > 4) {
                fprintf(stderr, "Warning: key value %lu for tag op out of bounds [0,3], ignoring\n", t->value_numeric);
                warn();
            } else {
                i->n_operands = t->value_numeric - 1;
            }
        } else if (strcmp(t->ident, "imm") == 0) {
            if (t->value_numeric == 0) {
                fprintf(stderr, "Warning: tag imm without numeric key value, ignoring\n");
                warn();
            } else if (t->value_numeric > 2) {
                fprintf(stderr, "Warning: key value %lu for tag imm out of bounds [0,2], ignoring\n", t->value_numeric);
                warn();
            } else {
                i->n_immediates = t->value_numeric - 1;
            }
        }

        t = t->next;
    }
    */

    if (idef_table) {
        idef_table = realloc(idef_table, sizeof(*idef_table) * (n_idefs + 1));
    } else {
        idef_table = malloc(sizeof(*idef_table));
    }

    idef_table[n_idefs++] = i;
}

void print_idefs() {
    for (uint64_t i = 0; i < n_idefs; i++) {
        printf("idef: %s\n", idef_table[i]->ident);
        printf("  bits = %lu\n", idef_table[i]->bits);
        printf("  n_operands = %lu\n", idef_table[i]->n_operands);
        printf("  n_immediates = %lu\n", idef_table[i]->n_immediates);
        printf("  tags:\n");
        
        tag *t = idef_table[i]->tags;

        while (t) {
            if (t->value_numeric > 0)
                printf("    %s = %lu\n", t->ident, t->value_numeric);
            else if (t->value_ident)
                printf("    %s = %s\n", t->ident, t->value_ident);
            else
                printf("    %s\n", t->ident);

            t = t->next;
        }
    }
}

idef* idef_lookup(char *ident) {
    for (uint64_t i = 0; i < n_idefs; i++) {
        if (strcmp(ident, idef_table[i]->ident) == 0)
            return idef_table[i];
    }

    return NULL;
}

int has_tag(idef *i, char *ident, uint64_t *value_numeric, char **value_ident) {
    tag *t = i->tags;

    while (t) {
        if (strcmp(ident, t->ident) == 0) {
            if (value_numeric)
                *value_numeric = t->value_numeric;
            if (value_ident)
                *value_ident = t->value_ident;
            return 1;
        }
        t = t->next;
    }

    return 0;
}

idef_info* idef_get_info(idef *def) {
    idef_info *info = malloc(sizeof(*info));

    //defaults
    info->n_operands = 3;
    info->n_immediates = 0;
    info->label_allowed = 1;

    //tags
    uint64_t tv_numeric = 0;
    char* tv_ident = NULL;

    if (has_tag(def, "op", &tv_numeric, &tv_ident)) {
        info->n_operands = tv_numeric - 1;
    }

    if (has_tag(def, "imm", &tv_numeric, &tv_ident)) {
        if (tv_numeric > 1) {
            info->n_immediates = tv_numeric - 1;
        } else if (tv_ident && strcmp(tv_ident, "short") == 0) {
            info->n_immediates = 1;
        } else if (tv_ident && strcmp(tv_ident, "long") == 0) {
            info->n_immediates = 2;
        }

        info->n_operands -= info->n_immediates;
    }

    if (has_tag(def, "nolabel", NULL, NULL)) {
        info->label_allowed = 0;
    }

    return info;
}

uint64_t get_definitions(idef ***table) {
    *table = idef_table;

    return n_idefs;
}

void set_microcode_bits(uint64_t n) {
    if (idef_max_bits > n) {
        fprintf(stderr, "Warning: option reduces bits below previous value (line %d)\n", yylineno);
        warn();
    }

    idef_max_bits = n;
}

uint64_t get_microcode_bits() {
    return idef_max_bits;
}

void set_option(char *ident, uint64_t value) {
    if (strcmp(ident, "bits") == 0) {
        set_microcode_bits(value);
    } else {
        fprintf(stderr, "Warning: unknown option %s (line %d)\n", ident, yylineno);
        warn();
    }
}
