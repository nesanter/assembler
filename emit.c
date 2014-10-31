#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "kasm.h"

void emit_instructions(FILE *f, int verbose, emit_format format) {
    inst *i = get_instructions();

    while (i) {
//        print_bin(encode_instruction(i), 32);
        printf("--");
        if (verbose) {
            printf(" // ");
            print_instruction(i);
        }
        printf("\n");

        if (i->next->address > i->address + 1) {
            printf("@ ");
            print_hex(f, i->next->address, 0);
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
    return 0;
}
