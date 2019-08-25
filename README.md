# Jgrandson

Jgrandson is a JSON parser and generator for the C language with a convenient getter/setter API.

* Modern API (C11), fully compliant with the current version of the JSON standard (as of August 2019: [RFC 8259](https://tools.ietf.org/html/rfc8259)
* Getter and setter functions for every common C type, each available in 3 forms: root element, array element, and object element. No need to cast!
* Extensive range of getter options customizable per function call through convenient optional arg struct type signatures, allowing custom boundary checking and
 custom error message contexts
* Unique error codes, informative error strings, and inclusion of highlighted parse error contexts
* Feature-complete optional args regarding escaping/unescaping of unicode and control characters, caller/callee-provided buffers, (non-)null-termination of strings, and whitespace generation
* Complete support for default key-value pairs: ideal for configuration file usage

## Table of contents

* [Installation](#installation)
* [Usage](#usage)
  * [Sessions](#sessions)
  * [Error handling](#error-handling)
* [Example](#example)
* [Todo](#todo)
* [Contributing](#contributing)
* [Email me](#email-me)

## Installation

Jgrandson has no external dependencies; so on Linux, with a fairly recent version of GCC installed, something like this should work:

    git clone https://github.com/wbudd/jgrandson.git
    cd jgrandson
    make
    sudo make install

On other platforms the Makefile will probably need some simple editing.

## Usage

The Jgrandson API consists exactly of the entire [jgrandson.h](https://github.com/wbudd/jgrandson/blob/master/src/jgrandson.h) header file, so see the many function definitions and comments contained therein for usage details not (yet) covered by this README.

### Sessions

A Jgrandson session is started by calling `jg_init()`, and terminated by calling `jg_free()`. During one such session Jgrandson supports either of the following two mutually exclusive workflows:

1) A single call to a `jg_parse_...()` function, followed by one or more getter calls of the form `jg_[root|arr|obj]_get_...()`.
2) One or more setter calls of the form `jg_[root|arr|obj]_set_...()`, followed by a single call to a `jg_generate_...()` function.

No state can be shared between these two workflows. While `jg_reinit()` can be used to switch from one to the other, doing so is functionally equivalent to calling `jg_free()` followed by `jg_init()`.

### Error handling

Every non-void Jgrandson function other than `jg_init()` returns the type [jg_ret](https://github.com/wbudd/jgrandson/blob/master/src/jgrandson.h), which is an `enum` that can equal `JG_OK` (zero) or an `JG_E_...` error value (greater than zero).

To obtain an error string associated with the last returned `jg_ret` error value, call `jg_get_err_str()`. The returned string should be treated read-only/`const`, and may be `free()`d by Jgrandson during a subsequent call to `jg_get_err_str()` or `jg_free()`.

## Example

To get started quickly, a self-contained example may be helpful. If so, consider the following [foo.c](https://github.com/wbudd/jgrandson/blob/master/example/foo.c), which should be compilable with something like `gcc -Wall -Wextra -std=c11 -ljgrandson foo.c`:
```C
#include <jgrandson.h>
#include <uchar.h> // Optional: enable unicode string literals (C11)

// Optional, but makes Jgrandson return value checking a little less cumbersome.
#define FOO_GUARD_JG(_ret_val) if ((_ret_val) != JG_OK) return -1

int main(void) {
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
```
Now let's assume the existence of some silly JSON file [foo.json](https://github.com/wbudd/jgrandson/blob/master/example/foo.json):
```JSON
{
  "strings?": ["Is\u0000超poor", "yet valid JSON", "\uD834\uDD1E？\n", true],
  "I am a 🔑": {
    "id": 18446744073709551614,
    "размер": 9876543210,
    "short_flo": 0.089,
    "long_flo": -5e+3
  }
}
```
...to be stored in an equally nonsensical C struct like this:
```C
struct {
    char * * iGadgetX;
    bool is_poor_example;
    struct {
        uint64_t id;
        size_t size;
        float short_flo;
        long double long_flo;
    }; // C11 anonymous struct
} foo = {0};
```
...with a function named `foo_parse_json()`:
```C
int foo_parse_json(jg_t * jg) {
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
        &(float){0.12345}, &foo.short_flo));
    FOO_GUARD_JG(jg_obj_get_long_double(jg, child_obj, "long_flo",
        &(long double){-1.2345}, &foo.long_flo));
    return 0;
}
```
Finally, let's generate a new JSON text from the obtained JSON data, and print the result with `foo_generate_json()`:
```C
int foo_generate_json(jg_t * jg) {
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
```
For "real-world" examples of Jgrandson usage, see [RingSocket](https://github.com/wbudd/ringsocket/blob/master/src/rs_conf.c) and [Realitree](https://github.com/wbudd/realitree/blob/master/realitree_ringsocket/src/rt_storage.c).

## Todo
* Create a test suite to test the many combinations of possible args/options.
* Implement optional args for floating point getters similar to those of integer type getters - requires *correct* floating comparisons.

## Contributing
Pull requests and other contributions are always welcome! License: [MIT](https://github.com/wbudd/jgrandson/blob/master/LICENSE).

## Email me
Feel free to send me email at the address below, *especially* if you might be interested in offering me employment or contractual work. Based in Osaka, Japan; but also happy to work remotely.

           _               _               _     _                   
          | |             | |             | |   | |                  
     _ _ _| |__      _ _ _| |__  _   _  __| | __| |  ____ ___  ____  
    | | | |  _ \    | | | |  _ \| | | |/ _  |/ _  | / ___) _ \|    \ 
    | | | | |_) ) @ | | | | |_) ) |_| ( (_| ( (_| |( (__| |_| | | | |
     \___/|____/     \___/|____/|____/ \____|\____(_)____)___/|_|_|_|
（日本語能力試験N1を取得／永住者の在留資格あり）
