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

No state can be shared between these two usage patterns. While `jg_reinit()` can be used to switch from one to the other, doing so is functionally equivalent to calling `jg_free()` followed by `jg_init()`.

Every Jgrandson function returns the type `jg_ret`, which is an `enum` that can equal `JG_OK` (zero) or an `JG_E_...` error value (greater than zero). To obtain an error string associated with the last returned `jg_ret` error value, call `jg_get_err_str()`. The returned string should be treated read-only/`const`, and may be `free()`d by Jgrandson during a subsequent call to `jg_get_err_str()` or `jg_free()`.

For the exact definition of the Jgrandson API and all its public prototypes refer to the `jgrandson.h` [header file](https://github.com/wbudd/jgrandson/blob/master/src/jgrandson.h).

### Parsing

A JSON text can be parsed with either of the following functions:

`jg_parse_file()`: Takes a file path string, reads the corresponding file as a JSON text, and attempts to parse it.

`jg_parse_str()`: Takes a string, copies its contents to a private `malloc`ed string buffer, and attempts to parse it.

`jg_parse_callerstr()`: Same as above, except that it parses the string as-is without making a copy. Note that the caller must guarantee that the string's storage duration lasts at least as long as the Jgrandson session (i.e., until `jg_free()` is called), and that the string is not altered during that time.

If the parse function returns `JG_OK`, the JSON text in question has been fully parsed and validated in accordance with [RFC 8259](https://tools.ietf.org/html/rfc8259).

### Getting

### Setting

### Generating

Once you've completed setting JSON values, a JSON text can be generated with either of the following functions:

`jg_parse_file()`

`jg_parse_str()`

`jg_parse_callerstr()`
