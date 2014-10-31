#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "kasm.h"

void emit_instructions(FILE *f, int verbose, emit_format format, char *secname) {
    inst *i = get_instructions(secname);

    while (i) {
        if (format == EF_MEMB)
            print_bin(f, encode_instruction(i), 32);
        else if (format == EF_MEMH)
            print_hex(f, encode_instruction(i), 32);
        if (verbose) {
            fprintf(f, " // ");
            print_instruction(f, i, 1);
        }
        fprintf(f, "\n");

        if (i->next && i->next->real_address > i->real_address + 1) {
            fprintf(f, "@ ");
            print_hex(f, i->next->real_address, 0);
            fprintf(f, "\n");
        }

        i = i->next;
    }
}

void emit_microcode(FILE *f, int verbose, emit_format format) {
    idef **table;
    uint64_t n_idefs = get_definitions(&table);

    for (uint64_t i = 0; i < n_idefs; i++) {
        if (format == EF_MEMB)
            print_bin(f, table[i]->bits, get_microcode_bits());
        else if (format == EF_MEMH)
            print_hex(f, table[i]->bits, get_microcode_bits());
        if (verbose)
            fprintf(f, " // %lu -- %s\n", i, table[i]->ident);
        else
            fprintf(f, "\n");
    }
}

void print_bin(FILE *f, uint64_t n, uint64_t bits) {
    char *s = malloc(65);
    char *p = &s[64];
    p[1] = '\0';

    for (uint64_t i = 0; i < bits; i++) {
        if (n & 1)
            *p-- = '1';
        else
            *p-- = '0';

        n >>= 1;
    }

    fprintf(f, "%s", p+1);
    free(s);
}

void print_hex(FILE *f, uint64_t n, uint64_t bits) {
    char *s = malloc(17);
    char *p = &s[16];
    p[1] = '\0';

    for (uint64_t i = 0; (i < (bits/4)) || (bits == 0); i++) {
        if ((n & 0xF) > 9)
            *p-- = 'A' - 10 + (n & 0xF);
        else
            *p-- = '0' + (n & 0xF);

        n >>= 4;

        if (bits == 0 && n == 0)
            break;
    }

    fprintf(f, "%s", p+1);
    free(s);
}

uint64_t encode_instruction(inst *i) {
    uint64_t n = 0;

    if (i->oper1) {
        n |= i->oper1->base;
        n |= encode_offset(i->oper1) << 4;
    }

    if (i->oper2) {
        n |= i->oper2->base << 7;
        n |= encode_offset(i->oper2) << 11;
    }

    if (i->oper3) {
        n |= i->oper3->base << 14;
        n |= encode_offset(i->oper3) << 18;
    }

    if (i->type == NONE) {
        //do nothing
    } else if (i->type == SINGLE) {
        if (i->immediate >= (1 << 7)) {
            fprintf(stderr, "Warning: short immediate %lu exceeds maximum, capping\n", i->immediate);
            warn();
            n |= 0x7F << 14;
        } else {
            n |= i->immediate << 14;
        }
    } else {
        if (i->immediate >= (1 << 14)) {
            if (i->immediate_ident)
                fprintf(stderr, "Warning: long immediate %s (%lu) exceeds maximum, capping\n", i->immediate_ident, i->immediate);
            else
                fprintf(stderr, "Warning: long immediate %lu exceeds maximum, capping\n", i->immediate);
            warn();
            n |= 0x3FFF << 7;
        } else {
            n |= i->immediate << 7;
        }
    }

    return (i->def->value << 21) | n;
}

uint64_t encode_offset(operand *o) {
    if (o->offset2 == 0)
        return o->offset1;
    if (o->offset1 == 1)
        return 2 + o->offset2;
    return 4 + o->offset2;
}
