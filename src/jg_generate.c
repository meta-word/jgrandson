// SPDX-License-Identifier: MIT
// Copyright © 2021 MetaWord Inc
// Copyright © 2019-2021 William Budd

#include "jgrandson_internal.h"

#define JG_PUT(_c) \
do { \
    if (json_text) { \
        json_text[*json_i] = (_c); \
    } \
    (*json_i)++; \
} while (0)

#define JG_PUT_STR(_str) \
do { \
    size_t byte_c = strlen(_str); \
    if (json_text) { \
        memcpy(json_text + *json_i, (_str), byte_c); \
    } \
    *json_i += byte_c; \
} while (0)

#define JG_PUT_SPACE \
do { \
    if (!opt->no_whitespace) { \
        JG_PUT(' '); \
    } \
} while (0)

#define JG_PUT_NEWLINE \
do { \
    if (!opt->no_whitespace) { \
        if (opt->include_cr) { \
            JG_PUT('\r'); \
        } \
        JG_PUT('\n'); \
    } \
} while (0)

#define JG_PUT_INDENT \
do { \
    if (!opt->no_whitespace) { \
        if (opt->indent_is_tab) { \
            for (size_t i = 0; i < *indent; i++) { \
                JG_PUT('\t'); \
            } \
        } else { \
            for (size_t i = 0; i < *indent; i++) { \
                JG_PUT(' '); \
            } \
        } \
    } \
} while (0)

#define JG_PUT_INDENT_INCR \
do { \
    *indent += *opt->indent; \
    JG_PUT_INDENT; \
} while(0)

#define JG_PUT_INDENT_DECR \
do { \
    *indent -= *opt->indent; \
    JG_PUT_INDENT; \
} while(0)

static void generate_json_text(
    struct jg_val_out const * v,
    char * const json_text,
    size_t * json_i,
    size_t * indent,
    jg_opt_whitespace * opt
) {
    switch (v->type) {
    case JG_TYPE_NULL:
        JG_PUT('n'); JG_PUT('u'); JG_PUT('l'); JG_PUT('l');
        return;
    case JG_TYPE_BOOL:
        if (v->bool_is_true) {
            JG_PUT('t'); JG_PUT('r'); JG_PUT('u'); JG_PUT('e');
        } else {
            JG_PUT('f'); JG_PUT('a'); JG_PUT('l'); JG_PUT('s'); JG_PUT('e');
        }
        return;
    case JG_TYPE_NUM:
        JG_PUT_STR(v->str);
        return;
    case JG_TYPE_STR:
        JG_PUT('"');
        JG_PUT_STR(v->str);
        JG_PUT('"');
        return;
    case JG_TYPE_ARR:
        JG_PUT('[');
        if (v->arr) {
            struct jg_arr_node * node = v->arr;
            generate_json_text(&node->elem, json_text, json_i, indent, opt);
            while ((node = node->next)) {
                JG_PUT(',');
                JG_PUT_SPACE;
                generate_json_text(&node->elem, json_text, json_i, indent, opt);
            }
        }
        JG_PUT(']');
        return;
    case JG_TYPE_OBJ: default:
        JG_PUT('{');
        if (v->obj) {
            struct jg_obj_node * node = v->obj;
            JG_PUT_NEWLINE;
            JG_PUT_INDENT_INCR;
            JG_PUT('"');
            JG_PUT_STR(node->key);
            JG_PUT('"');
            JG_PUT(':');
            JG_PUT_SPACE;
            generate_json_text(&node->val, json_text, json_i, indent, opt);
            while ((node = node->next)) {
                JG_PUT(',');
                JG_PUT_NEWLINE;
                JG_PUT_INDENT;
                JG_PUT('"');
                JG_PUT_STR(node->key);
                JG_PUT('"');
                JG_PUT(':');
                JG_PUT_SPACE;
                generate_json_text(&node->val, json_text, json_i, indent, opt);
            }
            JG_PUT_NEWLINE;
            JG_PUT_INDENT_DECR;
        }
        JG_PUT('}');
    }
}

jg_ret jg_generate_str(
    jg_t * jg,
    jg_opt_whitespace * opt,
    char * * json_text,
    size_t * _byte_c
) {
    if (jg->state != JG_STATE_SET && jg->state != JG_STATE_GENERATE) {
        return jg->ret = JG_E_STATE_NOT_GENERATE;
    }
    jg->state = JG_STATE_GENERATE;
    struct jg_opt_whitespace defa = {
        .indent = (size_t []){2},
        .indent_is_tab = false,
        .include_cr = false,
        .no_whitespace = false,
        .no_newline_before_eof = false
    };
    if (!opt) {
        opt = &defa;
    } else if (!opt->indent) {
        opt->indent = defa.indent;
    }
    size_t byte_c = 0;
    generate_json_text(&jg->root_out, NULL, &byte_c, (size_t []){0}, opt);
    byte_c += !opt->no_newline_before_eof;
    if (_byte_c) {
        *_byte_c = byte_c;
    }
    *json_text = malloc(byte_c + 1);
    if (!*json_text) {
        return jg->ret = JG_E_MALLOC;
    }
    if (!opt->no_newline_before_eof) {
        (*json_text)[byte_c - 1] = '\n';
    };
    (*json_text)[byte_c] = '\0';
    generate_json_text(&jg->root_out, *json_text, (size_t []){0},
        (size_t []){0}, opt);
    return JG_OK;
}

jg_ret jg_generate_callerstr(
    jg_t * jg,
    jg_opt_whitespace * opt,
    char * json_text,
    size_t * _byte_c
) {
    if (jg->state != JG_STATE_SET && jg->state != JG_STATE_GENERATE) {
        return jg->ret = JG_E_STATE_NOT_GENERATE;
    }
    jg->state = JG_STATE_GENERATE;
    struct jg_opt_whitespace defa = {
        .indent = (size_t []){2},
        .indent_is_tab = false,
        .include_cr = false,
        .no_whitespace = false,
        .no_newline_before_eof = false
    };
    if (!opt) {
        opt = &defa;
    } else if (!opt->indent) {
        opt->indent = defa.indent;
    }
    size_t byte_c = 0;
    generate_json_text(&jg->root_out, json_text, &byte_c, (size_t []){0}, opt);
    if (!opt->no_newline_before_eof) {
        if (json_text) {
            json_text[byte_c] = '\n';
        }
        byte_c++;
    }
    if (_byte_c) {
        *_byte_c = byte_c;
    }
    return JG_OK;
}

jg_ret jg_generate_file(
    jg_t * jg,
    jg_opt_whitespace * opt,
    char const * filepath
) {
    if (jg->state != JG_STATE_SET && jg->state != JG_STATE_GENERATE) {
        return jg->ret = JG_E_STATE_NOT_GENERATE;
    }
    jg->state = JG_STATE_GENERATE;
    char * json_text = NULL;
    size_t byte_c = 0;
    JG_GUARD(jg_generate_str(jg, opt, &json_text, &byte_c));
    FILE * f = fopen(filepath, "wb");
    if (!f) {
        jg->err_val.errn = errno;
        return jg->ret = JG_E_ERRNO_FOPEN;
    }
    if (fwrite(json_text, 1, byte_c, f) != byte_c) {
        free(json_text);
        return jg->ret = JG_E_FWRITE;
    }
    free(json_text);
    if (fclose(f)) {
        jg->err_val.errn = errno;
        return jg->ret = JG_E_ERRNO_FCLOSE;
    }
    return JG_OK;
}
