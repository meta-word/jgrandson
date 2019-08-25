// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#include "jgrandson_internal.h"

static jg_ret check_state_set(
    jg_t * jg
) {
    switch (jg->state) {
    case JG_STATE_INIT:
        jg->state = JG_STATE_SET;
        // fall through
    case JG_STATE_SET:
        return JG_OK;
    default:
        return jg->ret = JG_E_STATE_NOT_SET;
    }
}

static jg_ret check_root_not_set(
    jg_t * jg
) {
    return jg->root_out.type == JG_TYPE_NULL ? JG_OK :
        (jg->ret = JG_E_SET_ROOT_ALREADY_SET);
}

static jg_ret append_arr_node(
    jg_t * jg,
    struct jg_val_out * parent,
    struct jg_val_out * * child
) {
    if (parent->type != JG_TYPE_ARR) {
        return jg->ret = JG_E_SET_NOT_ARR;
    }
    struct jg_arr_node * node = parent->arr;
    if (node) {
        while (node->next) {
            node = node->next;
        }
        node = node->next = calloc(1, sizeof(struct jg_arr_node));
    } else {
        node = parent->arr = calloc(1, sizeof(struct jg_arr_node));
    }
    if (!node) {
        return jg->ret = JG_E_CALLOC;
    }
    *child = &node->elem;
    return JG_OK;
}

static jg_ret append_obj_node(
    jg_t * jg,
    struct jg_val_out * parent,
    char const * key,
    struct jg_val_out * * child
) {
    if (parent->type != JG_TYPE_OBJ) {
        return jg->ret = JG_E_SET_NOT_OBJ;
    }
    struct jg_obj_node * node = parent->obj;
    if (node) {
        while (node->next) {
            if (!strcmp(node->key, key)) {
                return jg->ret = JG_E_SET_OBJ_DUPLICATE_KEY;
            }
            node = node->next;
        }
        node = node->next = calloc(1, sizeof(struct jg_obj_node));
    } else {
        node = parent->obj = calloc(1, sizeof(struct jg_obj_node));
    }
    if (!node) {
        return jg->ret = JG_E_CALLOC;
    }
    JG_GUARD(alloc_strcpy(jg, &node->key, key, strlen(key)));
    *child = &node->val;
    return JG_OK;
}

static jg_ret escape_and_set_str(
    jg_t * jg,
    uint8_t const * unesc_str,
    size_t unesc_byte_c,
    struct jg_val_out * child
) {
    size_t byte_c = get_json_byte_c(unesc_str, unesc_byte_c);
    uint8_t * json_str = malloc(byte_c + 1);
    if (!json_str) {
        return jg->ret = JG_E_MALLOC;
    }
    json_str[byte_c] = '\0';
    unesc_str_to_json_str(unesc_str, unesc_byte_c, json_str);
    child->str = (char *) json_str;
    return JG_OK;
}

jg_ret jg_root_set_null(
    jg_t * jg
) {
    JG_GUARD(check_state_set(jg));
    JG_GUARD(check_root_not_set(jg));
    jg->root_out.type = JG_TYPE_NULL;
    return JG_OK;
}

jg_ret jg_arr_set_null(
    jg_t * jg,
    jg_arr_set_t * arr
) {
    JG_GUARD(check_state_set(jg));
    struct jg_val_out * child = NULL;
    JG_GUARD(append_arr_node(jg, arr, &child));
    child->type = JG_TYPE_NULL;
    return JG_OK;
}

jg_ret jg_obj_set_null(
    jg_t * jg,
    jg_obj_set_t * obj,
    char const * key
) {
    JG_GUARD(check_state_set(jg));
    struct jg_val_out * child = NULL;
    JG_GUARD(append_obj_node(jg, obj, key, &child));
    child->type = JG_TYPE_NULL;
    return JG_OK;
}

#define JG_SET_FUNC(_suf, _type, _json_type, _set_call) \
/* JG_[ROOT|ARR|OBJ]_SET prototype macros are defined in jgrandson.h */ \
JG_ROOT_SET(_suf, _type) { \
    JG_GUARD(check_state_set(jg)); \
    JG_GUARD(check_root_not_set(jg)); \
    struct jg_val_out * child = &jg->root_out; \
    child->type = _json_type; \
    _set_call; \
    return jg->ret = JG_OK; \
} \
\
JG_ARR_SET(_suf, _type) { \
    JG_GUARD(check_state_set(jg)); \
    struct jg_val_out * child = NULL; \
    JG_GUARD(append_arr_node(jg, arr, &child)); \
    child->type = _json_type; \
    _set_call; \
    return jg->ret = JG_OK; \
} \
\
JG_OBJ_SET(_suf, _type) { \
    JG_GUARD(check_state_set(jg)); \
    struct jg_val_out * child = NULL; \
    JG_GUARD(append_obj_node(jg, obj, key, &child)); \
    child->type = _json_type; \
    _set_call; \
    return jg->ret = JG_OK; \
}

JG_SET_FUNC(_arr, jg_arr_set_t * *, JG_TYPE_ARR, *v = child)

JG_SET_FUNC(_obj, jg_obj_set_t * *, JG_TYPE_OBJ, *v = child)

JG_SET_FUNC(_str, char const *, JG_TYPE_STR,
    JG_GUARD(escape_and_set_str(jg, (uint8_t *) v, strlen(v), child)))

JG_SET_FUNC(_json_str, char const *, JG_TYPE_STR,
    JG_GUARD(alloc_strcpy(jg, &child->str, v, strlen(v))))
JG_SET_FUNC(_json_callerstr, char const *, JG_TYPE_STR, child->callerstr = v)

JG_SET_FUNC(_bool, bool, JG_TYPE_BOOL, child->bool_is_true = v)

#define JG_SET_FUNC_NUM(_suf, _type, _num_fmt) \
    JG_SET_FUNC(_suf, _type, JG_TYPE_NUM, \
        JG_GUARD(print_alloc_str(jg, &child->str, "%" _num_fmt, v)))

JG_SET_FUNC_NUM(_int8, int8_t, PRId8)
JG_SET_FUNC_NUM(_char, char, "c")
JG_SET_FUNC_NUM(_signed_char, signed char, "hhd")
JG_SET_FUNC_NUM(_int16, int16_t, PRId16)
JG_SET_FUNC_NUM(_short, short, "hd")
JG_SET_FUNC_NUM(_int32, int32_t, PRId32)
JG_SET_FUNC_NUM(_int, int, "d")
JG_SET_FUNC_NUM(_int64, int64_t, PRId64)
JG_SET_FUNC_NUM(_long, long, "ld")
JG_SET_FUNC_NUM(_long_long, long long, "lld")
JG_SET_FUNC_NUM(_intmax, intmax_t, PRIdMAX)

JG_SET_FUNC_NUM(_uint8, uint8_t, PRIu8)
JG_SET_FUNC_NUM(_unsigned_char, unsigned char, "hhu")
JG_SET_FUNC_NUM(_uint16, uint16_t, PRIu16)
JG_SET_FUNC_NUM(_unsigned_short, unsigned short, "hu")
JG_SET_FUNC_NUM(_uint32, uint32_t, PRIu32)
JG_SET_FUNC_NUM(_unsigned, unsigned, "u")
JG_SET_FUNC_NUM(_uint64, uint64_t, PRIu64)
JG_SET_FUNC_NUM(_unsigned_long, unsigned long, "lu")
JG_SET_FUNC_NUM(_unsigned_long_long, unsigned long long, "llu")
JG_SET_FUNC_NUM(_sizet, size_t, "zu")
JG_SET_FUNC_NUM(_uintmax, uintmax_t, PRIuMAX)

JG_SET_FUNC_NUM(_float, float, "f")
JG_SET_FUNC_NUM(_double, double, "f")
JG_SET_FUNC_NUM(_long_double, long double, "Lf")
