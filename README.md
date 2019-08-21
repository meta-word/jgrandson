# Jgrandson

Jgrandson is a JSON parser and generator for C with a convenient getter/setter API.

## Installation

Jgrandson has no external dependencies; so on Linux, with a fairly recent version of GCC installed, something like this should work:

    git clone https://github.com/wbudd/jgrandson.git
    cd jgrandson
    make
    sudo make install

On other platforms the Makefile will probably need some simple editing.

## Usage

The Jgrandson API consists exactly of the entire [jgrandson.h](https://github.com/wbudd/jgrandson/blob/master/src/jgrandson.h) header file, so see the many function definitions and comments contained therein for more comprehensive usage information.

That said, to get started quickly, a self-contained example may be helpful. If so, consider the following `foo.c`, which can be compiled with something like `gcc -Wall -Wextra -std=c11 -ljgrandson foo.c`:
```C
#include <jgrandson.h>

// Optional, but makes Jgrandson return value checking a little less cumbersome.
#define FOO_GUARD_JG(_ret_val) if ((_ret_val) != JG_OK) return -1

int main(void) {
    // Initialize a Jgrandson session to obtain an opaque jg_t pointer. 
    jg_t * jg = jg_init(); // jg to be 1st arg to all following jg_...() calls

    int ret = foo_parse_json(jg); // See "Parsing and getting JSON" below.
    // Or: int ret = foo_generate_json(jg); // See "Setting and generating JSON".
    
    if (ret) {
        // Args 2 and 3 allow custom before and after error highlighting marker
        // strings. E.g., jg_get_err_str(jg, "-->", "<--").
        fprintf(stderr, "Jgrandson: %s\n", jg_get_err_str(jg, NULL, NULL));
    }

    jg_free(jg); // Free all data belonging to this Jgrandson session.
    return ret;
}
```
