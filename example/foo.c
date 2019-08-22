#include <jgrandson.h>

// Optional, but makes Jgrandson return value checking a little less cumbersome.
#define FOO_GUARD_JG(_ret_val) if ((_ret_val) != JG_OK) return -1

struct {
    char * * iGadgetX;
    bool isPoorExample;
    struct {
        uint64_t id;
        size_t size;
        float short_flo;
        long double long_flo;
    }; // C11 anonymous struct
} foo = {0};

int foo_parse_json(jg_t * jg) {
    ;
}

int foo_generate_json(jg_t * jg) {
    ;
}

int main(void) {
    // Initialize a Jgrandson session to obtain an opaque jg_t pointer. 
    jg_t * jg = jg_init(); // jg to be 1st arg to all following jg_...() calls

    int ret = foo_parse_json(jg); // See below.
    // Or: int ret = foo_generate_json(jg); // See below.
    
    if (ret) {
        // Args 2 and 3 allow custom before and after error highlighting marker
        // strings. E.g., jg_get_err_str(jg, "-->", "<--").
        fprintf(stderr, "Jgrandson: %s\n", jg_get_err_str(jg, NULL, NULL));
    }

    jg_free(jg); // Free all data belonging to this Jgrandson session.
    return ret;
}
