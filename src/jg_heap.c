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
    if (jg->err_str_needs_free) {
        free(jg->err_str);
        jg->err_str = NULL;
        jg->err_str_needs_free = false;
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

static void free_value_in(
    struct jg_val_in * v
) {
    switch (v->type) {
    case JG_TYPE_ARR:
        for (struct jg_val_in * elem = v->arr->elems;
            elem < v->arr->elems + v->arr->elem_c; elem++) {
            free_value_in(elem);
        }
        free(v->arr);
        return;
    case JG_TYPE_OBJ:
        for (struct jg_pair * pair = v->obj->pairs;
            pair < v->obj->pairs + v->obj->pair_c; pair++) {
            free_value_in(&pair->val);
        }
        free(v->obj);
        return;
    default:
        return;
    }
}

static void free_value_out(
    struct jg_val_out * v
) {
    switch (v->type) {
    case JG_TYPE_STR:
        if (v->str_is_callerstr) {
            return;
        }
        // fall through
    case JG_TYPE_NUM:
        free(v->str);
        return;
    case JG_TYPE_ARR:
        for (struct jg_arr_node * node = v->arr; node;) {
            free_value_out(&node->elem);
            struct jg_arr_node * node_next = node->next;
            free(node);
            node = node_next;
        }
        return;
    case JG_TYPE_OBJ:
        for (struct jg_obj_node * node = v->obj; node;) {
            free(node->key);
            free_value_out(&node->val);
            struct jg_obj_node * node_next = node->next;
            free(node);
            node = node_next;
        }
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
        free_value_out(&jg->root_out);
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
