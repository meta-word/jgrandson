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

Include the Jgrandson header with `#include <jgrandson.h>`, and pass the `-ljgrandson` linker flag to your compiler. Then start with `jg_t * jg = jg_init()` to obtain an opaque pointer type that every subsequent Jgrandson call expects to receive as its 1st argument. When you're done calling Jgrandson, make a final call to `jg_free(jg)` to free all memory associated with that session.

Between `jg_init()` and `jg_free()` Jgrandson supports two mutually exclusive usage patterns:

1) A single call to a `jg_parse_...()` function, followed by one or more getter calls of the form `jg_[root|arr|obj]_get_...()`.
2) One or more setter calls of the form `jg_[root|arr|obj]_set_...()`, followed by a single call to a `jg_generate_...()` function.

No state can be shared between these two usage patterns. While `jg_reinit()` can be used to switch from one to the other, doing is functionally equivalent to calling `jg_free()` followed by `jg_init()`.

### Parsing

A JSON text can be parsed with either of the following functions:

`jg_parse_file()`
`jg_parse_str()`
`jg_parse_callerstr()`

### Getting

### Setting

### Generating

Once you've completed setting JSON values, a JSON text can be generated with either of the following functions:

`jg_parse_file()`
`jg_parse_str()`
`jg_parse_callerstr()`
