# Jgrandson

Jgrandson is a JSON parser and generator for the C language with a convenient getter/setter API.

* [Installation](#installation)
* [Usage](#usage)
  * [Sessions](#sessions)
  * [Error handling](#error-handling)
* [Example](#example)
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

// Optional, but makes Jgrandson return value checking a little less cumbersome.
#define FOO_GUARD_JG(_ret_val) if ((_ret_val) != JG_OK) return -1

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
```
Now let's assume the existence of some silly JSON file [foo.json](https://github.com/wbudd/jgrandson/blob/master/example/foo.json):
```JSON
{
  "iGadgetX": ["Is\u0000超poor", "yet valid JSON", "\uD834\uDD1E？\n", true],
  "I am a 🔑": {
    "id": 18446744073709551615,
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
    bool isPoorExample;
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
    ;
}
```
Finally, let's also try the reverse workflow with `foo_generate_json()`:
```C
int foo_generate_json(jg_t * jg) {
    ;
}
```
## Contributing
Pull requests and other contributions are always welcome! License: MIT.
## Email me
Feel free to send me an email at the address below, especially if you might be interested in offering me employment or contractual work. Based in Osaka, Japan; but also happy to work remotely.

           _               _               _     _                   
          | |             | |             | |   | |                  
     _ _ _| |__      _ _ _| |__  _   _  __| | __| |  ____ ___  ____  
    | | | |  _ \    | | | |  _ \| | | |/ _  |/ _  | / ___) _ \|    \ 
    | | | | |_) ) @ | | | | |_) ) |_| ( (_| ( (_| |( (__| |_| | | | |
     \___/|____/     \___/|____/|____/ \____|\____(_)____)___/|_|_|_|
（日本語能力試験N1を取得／永住者の在留資格あり）
