#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "kasm.h"
#include "kasm.tab.h"
#include <string.h>
#include <getopt.h>

#define MAX_ERRORS (5)

//char **identifiers;
//unsigned long long int nidents;

char **defines;
unsigned long long int ndefines;

unsigned long long preproc_depth = 0;

int errcount = 0;
int werror = 0;

extern FILE *yyin;

int verbose = 0;
emit_format format;

int main(int argc, char **argv) {

    char *secname = NULL;
    char *outfname = NULL;
    char *ucoutfname = NULL;
    
    int minfo = 0;
    int massemble = 0;
    int mmicrocode = 0;

    int c;
    while (1) {
        static struct option long_options[] = 
        {
            {"verbose", no_argument, &verbose, 1},
            {"werror", no_argument, &werror, 1},
            {"info", no_argument, 0, 'i'},
            {"out", required_argument, 0, 'o'},
            {"assemble", optional_argument, 0, 'a'},
            {"microcode", optional_argument, 0, 'm'},
            {"dummy", no_argument, 0, 'd'},
            {"format", required_argument, 0, 'f'},
            {0, 0, 0, 0}
        };

        int option_index = 0;
        c = getopt_long(argc, argv, "io:a::m::dv", long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
            case 0:
                break;
            case 'i':
                minfo = 1;
                break;
            case 'a':
                massemble = 1;
                if (optarg) {
                    secname = strdup(optarg);
                }
                break;
            case 'm':
                mmicrocode = 1;
                if (optarg) {
                    ucoutfname = optarg;
                }
                break;
            case 'o':
                outfname = optarg;
                break;
            case 'd':
                if (minfo || massemble || mmicrocode)
                    fprintf(stderr, "Warning: --dummy overrides previous mode\n");
                minfo = 0;
                massemble = 0;
                mmicrocode = 0;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'f':
                if (strcmp(optarg, "memh") == 0) {
                    format = EF_MEMH;
                } else if (strcmp(optarg, "memb") == 0) {
                    format = EF_MEMB;
                } else if (strcmp(optarg, "tuple") == 0) {
                    format = EF_TUPLE;
                } else {
                    fprintf(stderr, "Warning: --format: unknown format\n");
                }
            case '?':
                break;
            default:
                printf("%c\n", c);
                exit(2);
        }
    }

    FILE *f = NULL;

    if (optind < argc) {
        f = fopen(argv[optind], "r");
        if (!f) {
            perror(argv[optind]);
            return 1;
        }

        yyin = f;
    }

    if (yyparse()) {
        if (f)
            fclose(f);
        return 1;
    }
    
    if (f)
        fclose(f);
/*
    print_bitdefs();
    print_idefs();
    print_sections();
    print_sections_contents();

    inst *i = get_instructions();
    while (i) {
        printf("%s (%llu)\n", i->def->ident, i->real_address);
        i = i->next;
    }
*/
    if (minfo) {
        print_bitdefs();
        print_idefs();
        print_sections();
        print_sections_contents();
    }
    if (massemble) {
        if (verbose) {
            if (secname)
                printf("(assemble section %s)\n", secname);
            else
                printf("(assemble all)\n");
        }

        FILE *out;
        if (outfname) {
            out = fopen(outfname, "w");

            if (!f) {
                perror(ucoutfname);
                return 1;
            }
        } else {
            out = stdout;
        }
        
        emit_instructions(out, verbose, format, secname);
    }
    if (mmicrocode) {
        if (verbose) {
            printf("(generate microcode)\n");
        }
        FILE *ucout;
        if (ucoutfname) {
            ucout = fopen(ucoutfname, "w");

            if (!ucout) {
                perror(ucoutfname);
                return 1;
            }

        } else {
            ucout = stdout;
        }

        emit_microcode(ucout, verbose, format);
    }
}

void warn() {
    if (werror) {
        fprintf(stderr, "Aborting because of prior warning.\n");
        exit(1);
    }
}

/*

ast* create_node(node_type type, unsigned long long value, unsigned int children, ...) {
    ast *n = malloc(sizeof(*n));

    va_list v;
    va_start(v, children);

    n->type = type;
    n->child = malloc(sizeof(ast*) * children);
    n->children = children;
    n->next = NULL;
    n->value = value;

    for (unsigned int i = 0; i < children; i++)
        n->child[i] = va_arg(v, ast*);

    va_end(v);
    return n;
}

unsigned long long int create_ident(char *s) {
    if (identifiers)
        identifiers = realloc(identifiers, sizeof(*identifiers) * (nidents + 1));
    else
        identifiers = malloc(sizeof(*identifiers));

    identifiers[nidents] = strdup(s);

    return nidents++;
}

void set_next(ast *node, ast *next) {
    node->next = next;
}
*/

void yyerror(char *s, ...) {
    if (++errcount > MAX_ERRORS) {
        fprintf(stderr, "Error count exceeded threshold. Aborting.");
        exit(1);
    }
}

/*
char* get_ident(unsigned long long int ident) {
    return identifiers[ident];
}

int main() {
    return yyparse();
}

void print_tree(ast *root, int depth) {
    for (int i = 0; i < depth; i++)
        printf("  ");
    
    if (!root) {
        printf("(null)\n");
        return;
    }

    switch (root->type) {
        case BITDEF:
            printf("BITDEF: %s\n", get_ident(root->value));
            break;
        case BITDEF_BITS:
            printf("BIT: %llu\n", root->value);
            break;
        case IDEF:
            printf("IDEF: %s\n", get_ident(root->value));
            break;
        case BIT_IMMEDIATE:
            printf("USE BIT: %llu\n", root->value);
            break;
        case BIT_DEFINED:
            printf("USE BITDEF: %s\n", get_ident(root->value));
            break;
        case TAGNAME:
            printf("TAG NAME: %s\n", get_ident(root->value));
            break;
        case TAGVAL_IDENT:
            printf("TAG VALUE: %s\n", get_ident(root->value));
            break;
        case TAGVAL_NUMERIC:
            printf("TAG VALUE: %llu\n", root->value);
            break;
        case LABEL:
            printf("LABEL: %s\n", get_ident(root->value));
            break;
        case INST:
            printf("INST: %s\n", get_ident(root->value));
            break;
        case OPERAND:
            printf("OPERAND:\n");
            break;
        case OFFSET:
            printf("OFFSET: %llu\n", root->value);
            break;
        case REGISTER:
            printf("REGISTER: %s\n", get_ident(root->value));
            break;
        default:
            printf("UNKNOWN\n");
            break;
    }

    for (unsigned int i = 0; i < root->children; i++) {
        print_tree(root->child[i], depth + 1);
        for (int i = 0; i < depth; i++)
            printf("  ");
        printf("  ----\n");
    }
    if (root->next)
        print_tree(root->next, depth);
}
*/

void preproc_define(char *s) {
    if (defines)
        defines = realloc(defines, sizeof(*defines) * (ndefines + 1));
    else
        defines = malloc(sizeof(*defines));

    defines[ndefines] = strdup(s);

    ndefines++;
}

int preproc_isdefined(char *s) {
    for (unsigned long long int i = 0; i < ndefines; i++) {
        if (strcmp(s, defines[i]) == 0)
            return 1;
    }
    return 0;
}

void preproc_incdepth() {
    preproc_depth++;
}

int preproc_decdepth() {
    if (preproc_depth == 0)
        return 1;

    preproc_depth--;

    return 0;
}
