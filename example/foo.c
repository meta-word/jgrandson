#include <jgrandson.h>
#include <uchar.h> // Optional: enable unicode string literals (C11)

// Optional, but makes Jgrandson return value checking a little less cumbersome.
#define FOO_GUARD_JG(_ret_val) if ((_ret_val) != JG_OK) return -1

struct {
    char * * strings;
    size_t string_c;
    bool is_poor_example;
    struct {
        uint64_t id;
        size_t dimension;
        float short_flo;
        long double long_flo;
    }; // C11 anonymous struct
} foo = {0};

static int foo_parse_json(jg_t * jg) {
    // Instead of parsing a file as a JSON text, Jgrandson also allows parsing
    // a string as JSON text with jg_parse_str() or jg_parse_callerstr().
    FOO_GUARD_JG(jg_parse_file(jg, "foo.json"));
    // A return value of JG_OK means that the entire JSON text was successfully
    // parsed and validated.

    // Get a reference to the root JSON object. Note: use jg_root_get_arr()/
    // jg_root_get_str()/etc if your root JSON value is an array/string/etc.
    jg_obj_get_t * root_obj = NULL; // Destination type of ..._get_obj() funcs
    // (The following NULL omits optional args with regard to the JSON object.
    // For an example including them, see the jg_obj_get_obj_defa() call below.)
    FOO_GUARD_JG(jg_root_get_obj(jg, NULL, &root_obj));

    jg_arr_get_t * arr = NULL; // Destination type of ..._get_arr() funcs
    // The 4th argument below is a pointer to a struct (as a reference to a
    // struct literal) allowing optional arguments to be specified.
    // Many Jgrandson getters make use of the same construction, altough various
    // differences exist between the struct members they contain. The type of
    // this argument always mirrors the name of the function it belongs to:
    // jg_FOO_get_BAR() would require jg_FOO_BAR * as its optional arg type.
    FOO_GUARD_JG(jg_obj_get_arr(jg, root_obj, "strings?", &(jg_obj_arr){
        // Pointer to a struct literal containing one or more optional arguments
        .min_c = 2, // Require the JSON array to consist of at least 2 elements.
        .max_c = 100, // Set some max. (A .max_c of 0 is treated as infinity.)
        // Custom strings to be included by jg_get_err_str() if bound exceeded
        .min_c_reason = "Foo requires an array of at least 1 string and 1 bool",
        .max_c_reason = "Foo can't handle more than 100 values"
    }, &arr, &foo.string_c));
    // (Having JSON arrays with elements of different types is a terrible idea.
    //  This example does so only as a reminder that it's nonetheless valid
    //  JSON, and that Jgrandson therefore fully supports handling such cases.)
    // The last array element is expected to be a bool, so decrement foo.str_c.
    foo.strings = malloc(--foo.string_c * sizeof(char *)); // Get char ptr array
    if (!foo.strings) {
        fprintf(stderr, "Unsuccessful malloc()\n");
        return -2;
    }
    // In this case the JSON array elements are expected to all be of the same
    // type (string), so simply use a for loop:
    for (size_t i = 0; i < foo.string_c; i++) {
        // Note: to keep JSON strings escaped, use jg_arr_get_json_str() instead
        // Note: to supply a pre-allocated destination string buffer instead of
        // receiving a malloc()ed string use jg_arr_get_(json)_callerstr().
        FOO_GUARD_JG(jg_arr_get_str(jg, arr, i, &(jg_arr_str){
            // If "", set dest to NULL instead of allocating a contentless array
            .nullify_empty_str = true
        }, &foo.strings[i]));
    }
    // jg_[root|arr|obj]_get_bool() doesn't have an optional args parameter.
    FOO_GUARD_JG(jg_arr_get_bool(jg, arr, foo.string_c, &foo.is_poor_example));

    jg_obj_get_t * child_obj = NULL;
    char * * keys = NULL;
    size_t key_c = 0;
    // jg_obj_get_obj_defa() is the same as jg_obj_get_obj(), except that an
    // empty object is returned when the requested key-value pair doesn't exist.
    FOO_GUARD_JG(jg_obj_get_obj_defa(jg, root_obj,
        u8"I am a 🔑" /* C11 UTF-8 literal */, &(jg_obj_obj_defa){ // Opt args
        .keys = &keys, // Receive an array of heap-allocated strings of all keys
        .key_c = &key_c, // Set key_c to the number of keys in this object
        .max_c = 4, // Max key-val pair count (alike .max_c ..._get_arr option)
        .max_c_reason = u8"No keys are recognized other than \"id\", "
            "\"размер\", \"short_flo\", and \"long_flo\"."
    }, &child_obj));
    // <...do something with the keys array here, such as strcmp()ing them...>
    // The keys array must be free()d by the caller. Note though that its key
    // string elements (if any) must NOT be free()d, because they are part of
    // the same keys array (i.e., tail-concatenated one after another).
    free(keys); keys = NULL;
    FOO_GUARD_JG(jg_obj_get_uint64(jg, child_obj, "id",
        &(jg_obj_uint64){
        .defa = &(uint64_t){42}, // A default value of 42 (for whatever reason)
        .min = &(uint64_t){1}, // Some minimum value
        .min_reason = "0 is a reserved value." // Or some such
    }, &foo.id));
    FOO_GUARD_JG(jg_obj_get_sizet(jg, child_obj, u8"размер", &(jg_obj_sizet){
        .defa = &(size_t){24} // A default value of 24 (for whatever reason)
    }, &foo.dimension));
    // Floating point getters currently don't take an optional arg parameter
    // (although that may change in a future version). When getting from an
    // object, a default can be specified directly instead:
    FOO_GUARD_JG(jg_obj_get_float(jg, child_obj, "short_flo",
        &(float){0.12345f}, &foo.short_flo));
    FOO_GUARD_JG(jg_obj_get_long_double(jg, child_obj, "long_flo",
        &(long double){-1.2345}, &foo.long_flo));
    return 0;
}

static int foo_generate_json(jg_t * jg) {
    jg_obj_set_t * root_obj = NULL; // Note: ..._set_t differs from ..._get_t
    FOO_GUARD_JG(jg_root_set_obj(jg, &root_obj));

    jg_arr_set_t * arr = NULL;
    FOO_GUARD_JG(jg_obj_set_arr(jg, root_obj, "strings", &arr));
    for (size_t i = 0; i < foo.string_c; i++) {
        // Unlike jg_arr_get_...(), jg_arr_set_...() does not take an array
        // index. Each value set is simply appended to the end of the array.
        FOO_GUARD_JG(jg_arr_set_str(jg, arr, foo.strings[i]));
    }
    // Append the bool to the stringy array to mirror foo.json absurdness.
    FOO_GUARD_JG(jg_arr_set_bool(jg, arr, foo.strings[foo.string_c]));
 
    jg_obj_set_t * child_obj = NULL;
    FOO_GUARD_JG(jg_obj_set_obj(jg, root_obj, u8"I am a 🔑", &child_obj));
    FOO_GUARD_JG(jg_obj_set_uint64(jg, child_obj, "id", foo.id));
    FOO_GUARD_JG(jg_obj_set_sizet(jg, child_obj, u8"размер", foo.dimension));
    FOO_GUARD_JG(jg_obj_set_float(jg, child_obj, "short_flo", foo.short_flo));
    FOO_GUARD_JG(jg_obj_set_long_double(jg, child_obj, "long_flo",
        foo.long_flo));

    char * json_text = NULL;
    // The 2nd arg allows customizing various aspects of how whitespace is
    // generated (or omitted). If not NULL, the last arg will be set to the
    // byte count of the generated JSON.
    FOO_GUARD_JG(jg_generate_str(jg, NULL, &json_text, NULL));
    // Also available: jg_generate_file(), jg_generate_callerstr()
    printf("Generated JSON text:\n%s", json_text);
    return 0;
}

int main(void) {
#if defined(_WIN32) || defined(_WIN64)
    if (!SetConsoleOutputCP(CP_UTF8)) {
        fprintf(stderr, "Failed to SetConsoleOutputCP(CP_UTF8).\n");
        return EXIT_FAILURE;
    }
#endif
    // Initialize a Jgrandson session to obtain an opaque jg_t pointer. 
    jg_t * jg = jg_init(); // jg to be 1st arg to all following jg_...() calls

    int ret = foo_parse_json(jg); // See below.
    if (!ret) {
        jg_reinit(jg); // Equivalent to: jg_free(jg); jg = jg_init();
        ret = foo_generate_json(jg); // See below.
    }
    
    if (ret == -1) {
        // Args 2 and 3 allow custom before and after error highlighting marker
        // strings. E.g., jg_get_err_str(jg, "-->", "<--").
        fprintf(stderr, "Jgrandson: %s\n", jg_get_err_str(jg, NULL, NULL));
    }

    jg_free(jg); // Free all data belonging to this Jgrandson session.
    return ret;
}
