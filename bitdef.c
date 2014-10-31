#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "kasm.h"

bitdef **bitdef_table = NULL;
uint64_t n_bitdefs = 0;

uint64_t create_bitdef(uint64_t bit) {
    if (bit > 63) {
        fprintf(stderr, "Warning: bit specification %lu beyond allowed maximum, treating as 0 (line %d)\n", bit, yylineno);
        warn();
        return 0;
    }

    return (1 << bit);
}

uint64_t merge_bitdef(uint64_t def, uint64_t bit) {
    if (bit > 63) {
        fprintf(stderr, "Warning: bit specification %lu beyond allowed maximum, treating as 0 (line %d)\n", bit, yylineno);
        warn();
        return def;
    } else if (def & (1 << bit)) {
        fprintf(stderr, "Warning: redundant bit specification %lu, ignoring (line %d)\n", bit, yylineno);
        warn();
        return def;
    }

    return def | (1 << bit);
}

uint64_t merge_bitdef2(uint64_t def, uint64_t def2) {
    if (def & def2) {
        fprintf(stderr, "Warning: redundant bit specification, ignoring (line %d)\n", yylineno);
        warn();
    }

    return def | def2;
}

void register_bitdef(char *ident, uint64_t def) {
    if (bitdef_table) {
        bitdef_table = realloc(bitdef_table, sizeof(*bitdef_table) * (n_bitdefs + 1));
    } else {
        bitdef_table = malloc(sizeof(*bitdef_table));
    }

    bitdef *d = malloc(sizeof(*d));

    d->ident = strdup(ident);
    d->bits = def;

    bitdef_table[n_bitdefs++] = d;
}

uint64_t bitdef_lookup(char *ident) {
    for (uint64_t i = 0; i < n_bitdefs; i++) {
        if (strcmp(ident, bitdef_table[i]->ident)) {
            return bitdef_table[i]->bits;
        }
    }

    fprintf(stderr, "Warning: unknown bit definition %s, treating as null (line %d)\n", ident, yylineno);
    warn();

    return 0;
}

void print_bitdefs() {
    for (uint64_t i = 0; i < n_bitdefs; i++) {
        printf("bitdef: %s = %lu\n", bitdef_table[i]->ident, bitdef_table[i]->bits);
    }
}
