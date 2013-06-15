#include <stdio.h>
#include <htaccess/htaccess.h>


int
main (int argc, char *argv[]) {
    htaccess_ctx_t *ht_ctx;
    const char *fname = NULL;

    if (argc != 2) {
        printf("unknown amount of arguments, nothing to test with\n");
        return 1;
    }
    fname = argv[1];

    ht_ctx = new_htaccess_ctx();
    if (!ht_ctx)
        return 1;

    if (htaccess_parse_file(ht_ctx, fname) != 0) {
        printf("htaccess_parse_file() failed! Error: %s\n", htaccess_get_error(ht_ctx));
    }


    htaccess_print_ctx(ht_ctx);

    return 0;
}

