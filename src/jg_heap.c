// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#include "jgrandson_internal.h"

jg_t * jg_init(
    void
) {
    return calloc(1, sizeof(struct jgrandson));
}

void free_json_text(
    jg_t * jg
) {
    if (!jg->json_is_callertext && jg->json_text) {
        free(jg->json_text);
        jg->json_text = NULL;
    }
}

void free_err_str(
    jg_t * jg
) {
    if (!jg->err_str_is_static && jg->err_str) {
        free(jg->err_str);
        jg->err_str = NULL;
    }
}

jg_ret set_custom_err_str(
    jg_t * jg,
    char const * custom_err_str
) {
    size_t byte_c = strlen(custom_err_str) + 1;
    if (jg->custom_err_str) {
        jg->custom_err_str = realloc(jg->custom_err_str, byte_c);
        if (!jg->custom_err_str) {
            return jg->ret = JG_E_REALLOC;
        }
    } else {
        jg->custom_err_str = malloc(byte_c);
        if (!jg->custom_err_str) {
            return jg->ret = JG_E_MALLOC;
        }
    }
    memcpy(jg->custom_err_str, custom_err_str, byte_c);
    return JG_OK;
}

jg_ret alloc_strcpy(
    jg_t * jg,
    char * * dst,
    char const * src,
    size_t byte_c
) {
    *dst = malloc(byte_c + 1);
    if (*dst) {
        return jg->ret = JG_E_MALLOC;
    }
    memcpy(*dst, src, byte_c);
    (*dst)[byte_c] = '\0';
    return JG_OK;
}

// Needed because mutually recursive with free_[array|object]()
static void free_value_in(
    struct jg_val_in * v
);

static void free_array(
    struct jg_arr * arr
) {
    for (struct jg_val_in * elem = arr->elems;
        elem < arr->elems + arr->elem_c; elem++) {
        free_value_in(elem);
    }
    free(arr);
}

static void free_object(
    struct jg_obj * obj
) {
    for (struct jg_pair * pair = obj->pairs;
        pair < obj->pairs + obj->pair_c; pair++) {
        free_value_in(&pair->val);
    }
    free(obj);
}

static void free_value_in(
    struct jg_val_in * v
) {
    switch (v->type) {
    case JG_TYPE_ARR:
        free_array(v->arr);
        return;
    case JG_TYPE_OBJ:
        free_object(v->obj);
        return;
    default:
        return;
    }
}

static void free_all(
    jg_t * jg,
    bool free_jg
) {
    switch (jg->state) {
    case JG_STATE_INIT:
        if (free_jg) {
            free(jg);
        }
        return;
    case JG_STATE_PARSE:
    case JG_STATE_GET:
        free_value_in(&jg->root_in);
        break;
    case JG_STATE_SET:
    case JG_STATE_GENERATE: default:
        ; // todo: free_value_out(&jg->root_out);
    }
    free_json_text(jg);
    free_err_str(jg);
    if (jg->custom_err_str) {
        free(jg->custom_err_str);
    }
    if (free_jg) {
        free(jg);
    } else {
        memset(jg, 0, sizeof(*jg));
    }
}

void jg_free(
    jg_t * jg
) {
    free_all(jg, true);
}

void jg_reinit(
    jg_t * jg
) {
    free_all(jg, false);
}
