// SPDX-License-Identifier: MIT
// Copyright © 2019 William Budd

#include "jgrandson_internal.h"

static jg_ret check_state_get(
    jg_t * jg
) {
    switch (jg->state) {
    case JG_STATE_PARSE:
        jg->state = JG_STATE_GET;
        // fall through
    case JG_STATE_GET:
        return JG_OK;
    default:
        return jg->ret = JG_E_STATE_NOT_GET;
    }
}

static jg_ret check_type(
    jg_t * jg,
    struct jg_val_in const * child,
    enum jg_type type
) {
    if (child->type != type) {
        switch (child->type) {
        case JG_TYPE_ARR:
            jg->json_cur = child->arr->json;
            break;
        case JG_TYPE_OBJ:
            jg->json_cur = child->obj->json;
            break;
        default:
            jg->json_cur = child->json;
        }
        switch (type) {
        case JG_TYPE_NULL: return jg->ret = JG_E_GET_NOT_NULL;
        case JG_TYPE_BOOL: return jg->ret = JG_E_GET_NOT_BOOL;
        case JG_TYPE_NUM: return jg->ret = JG_E_GET_NOT_NUM;
        case JG_TYPE_STR: return jg->ret = JG_E_GET_NOT_STR;
        case JG_TYPE_ARR: return jg->ret = JG_E_GET_NOT_ARR;
        case JG_TYPE_OBJ: default: return jg->ret = JG_E_GET_NOT_OBJ;
        }
    }
    return JG_OK;
}

static jg_ret check_arr_index_over(
    jg_t * jg,
    struct jg_arr const * arr,
    size_t arr_i
) {
    if (arr->elem_c <= arr_i) {
        jg->err_val.s = arr_i;
        jg->json_cur = arr->json;
        return jg->ret = JG_E_GET_ARR_INDEX_OVER;
    }
    return JG_OK;
}

static jg_ret obj_get_val_by_key(
    jg_t * jg,
    struct jg_obj const * obj,
    char const * key,
    bool is_required,
    struct jg_val_in const * * val
) {
    size_t byte_c = strlen(key);
    for (struct jg_pair const * p = obj->pairs; p < obj->pairs + obj->pair_c;
        p++) {
        bool strings_are_equal = false;
        JG_GUARD(unesc_str_and_json_str_are_equal((uint8_t const *) key, byte_c,
            (uint8_t const *) p->key.json, p->key.byte_c, &strings_are_equal));
        if (strings_are_equal) {
            *val = &p->val;
            return JG_OK;
        }
    }
    if (is_required) {
        jg->json_cur = obj->json;
        // Not actually "custom" in this case, but use this anyway to save the
        // key string for jg_get_err_str().
        set_custom_err_str(jg, key);
        return jg->ret = JG_E_GET_OBJ_KEY_NOT_FOUND;
    }
    *val = NULL;
    return JG_OK;
}

////////////////////////////////////////////////////////////////////////////////
// ..._get_json_type() /////////////////////////////////////////////////////////

jg_ret jg_root_get_json_type(
    jg_t * jg,
    enum jg_type * type
) {
    JG_GUARD(check_state_get(jg));
    *type = jg->root_in.type;
    return jg->ret = JG_OK;
}

jg_ret jg_arr_get_json_type(
    jg_t * jg,
    struct jg_arr const * arr,
    size_t arr_i,
    enum jg_type * type
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_arr_index_over(jg, arr, arr_i));
    *type = arr->elems[arr_i].type;
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_json_type(
    jg_t * jg,
    struct jg_obj const * obj,
    char const * key,
    enum jg_type * type
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * child = NULL;
    JG_GUARD(obj_get_val_by_key(jg, obj, key, true, &child));
    *type = child->type;
    return jg->ret = JG_OK;
}

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_bool() ////////////////////////////////////////////////

jg_ret jg_root_get_bool(
    jg_t * jg,
    bool * v
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_type(jg, &jg->root_in, JG_TYPE_BOOL));
    *v = jg->root_in.bool_is_true;
    return jg->ret = JG_OK;
}

jg_ret jg_arr_get_bool(
    jg_t * jg,
    struct jg_arr const * arr,
    size_t arr_i,
    bool * v
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_arr_index_over(jg, arr, arr_i));
    struct jg_val_in const * child = arr->elems + arr_i;
    JG_GUARD(check_type(jg, child, JG_TYPE_BOOL));
    *v = child->bool_is_true;
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_bool(
    jg_t * jg,
    struct jg_obj const * obj,
    char const * key,
    bool const * defa,
    bool * v
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * child = NULL;
    JG_GUARD(obj_get_val_by_key(jg, obj, key, !defa, &child));
    if (child) {
        JG_GUARD(check_type(jg, child, JG_TYPE_BOOL));
        *v = child->bool_is_true;
    } else {
        *v = *defa;
    }
    return jg->ret = JG_OK;
}

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_null() ////////////////////////////////////////////////

jg_ret jg_root_get_null(
    jg_t * jg
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_type(jg, &jg->root_in, JG_TYPE_NULL));
    return jg->ret = JG_OK;
}

jg_ret jg_arr_get_null(
    jg_t * jg,
    struct jg_arr const * arr,
    size_t arr_i
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_arr_index_over(jg, arr, arr_i));
    JG_GUARD(check_type(jg, arr->elems + arr_i, JG_TYPE_NULL));
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_null(
    jg_t * jg,
    struct jg_obj const * obj,
    char const * key
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * child = NULL;
    JG_GUARD(obj_get_val_by_key(jg, obj, key, true, &child));
    JG_GUARD(check_type(jg, child, JG_TYPE_NULL));
    return jg->ret = JG_OK;
}

////////////////////////////////////////////////////////////////////////////////
// ..._get_arr...() ////////////////////////////////////////////////////////////

static jg_ret handle_arr_options(
    jg_t * jg,
    struct jg_arr const * arr,
    char const * min_c_reason,
    char const * max_c_reason,
    size_t min_c,
    size_t max_c
) {
    if (arr->elem_c < min_c) {
        jg->err_val.s = min_c;
        jg->json_cur = arr->json;
        set_custom_err_str(jg, min_c_reason);
        return jg->ret = JG_E_GET_ARR_TOO_SHORT;
    }
    if (max_c && arr->elem_c > max_c) {
        jg->err_val.s = max_c;
        jg->json_cur = arr->json;
        set_custom_err_str(jg, max_c_reason);
        return jg->ret = JG_E_GET_ARR_TOO_LONG;
    }
    return JG_OK;
}

jg_ret jg_root_get_arr(
    jg_t * jg,
    jg_root_arr * opt,
    struct jg_arr const * * v,
    size_t * elem_c
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_type(jg, &jg->root_in, JG_TYPE_ARR));
    if (opt) {
        JG_GUARD(handle_arr_options(jg, jg->root_in.arr, opt->min_c_reason,
            opt->max_c_reason, opt->min_c, opt->max_c));
    }
    *v = jg->root_in.arr;
    *elem_c = jg->root_in.arr->elem_c;
    return jg->ret = JG_OK;
}

jg_ret jg_arr_get_arr(
    jg_t * jg,
    struct jg_arr const * arr,
    size_t arr_i,
    jg_arr_arr * opt,
    struct jg_arr const * * v,
    size_t * elem_c
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_arr_index_over(jg, arr, arr_i));
    struct jg_val_in const * child = arr->elems + arr_i;
    JG_GUARD(check_type(jg, child, JG_TYPE_ARR));
    if (opt) {
        JG_GUARD(handle_arr_options(jg, child->arr, opt->min_c_reason,
            opt->max_c_reason, opt->min_c, opt->max_c));
    }
    *v = child->arr;
    *elem_c = child->arr->elem_c;
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_arr(
    jg_t * jg,
    struct jg_obj const * obj,
    char const * key,
    jg_obj_arr * opt,
    struct jg_arr const * * v,
    size_t * elem_c
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * child = NULL;
    JG_GUARD(obj_get_val_by_key(jg, obj, key, true, &child));
    JG_GUARD(check_type(jg, child, JG_TYPE_ARR));
    if (opt) {
        JG_GUARD(handle_arr_options(jg, child->arr, opt->min_c_reason,
            opt->max_c_reason, opt->min_c, opt->max_c));
    }
    *v = child->arr;
    *elem_c = child->arr->elem_c;
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_arr_defa(
    jg_t * jg,
    struct jg_obj const * obj,
    char const * key,
    jg_obj_arr_defa * opt,
    struct jg_arr const * * v,
    size_t * elem_c
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * child = NULL;
    JG_GUARD(obj_get_val_by_key(jg, obj, key, false, &child));
    if (child) {
        JG_GUARD(check_type(jg, child, JG_TYPE_ARR));
        if (opt) {
            JG_GUARD(handle_arr_options(jg, child->arr, NULL, opt->max_c_reason,
                0, opt->max_c));
        }
        *v = child->arr;
        *elem_c = child->arr->elem_c;
    } else {
        static struct jg_arr empty_arr = {0};
        *v = &empty_arr;
        *elem_c = 0;
    }
    return jg->ret = JG_OK;
}

////////////////////////////////////////////////////////////////////////////////
// ..._get_obj...() ////////////////////////////////////////////////////////////

static jg_ret handle_obj_options(
    jg_t * jg,
    struct jg_obj const * obj,
    char * * * keys,
    size_t * key_c,
    char const * min_c_reason,
    char const * max_c_reason,
    size_t min_c,
    size_t max_c
) {
    if (obj->pair_c < min_c) {
        jg->err_val.s = min_c;
        jg->json_cur = obj->json;
        set_custom_err_str(jg, min_c_reason);
        return jg->ret = JG_E_GET_OBJ_TOO_SHORT;
    }
    if (max_c && obj->pair_c > max_c) {
        jg->err_val.s = max_c;
        jg->json_cur = obj->json;
        set_custom_err_str(jg, max_c_reason);
        return jg->ret = JG_E_GET_OBJ_TOO_LONG;
    }
    if (keys && key_c) {
        // The caller wants to receive all the keys to this object: allocate a
        // single heap buffer and fill it with pointers to each key string copy,
        // followed by said key strings themselves (i.e., have the buffer header
        // point to string locations within the tail of the same buffer). Aside
        // from being memory efficient, this causes the least work for the
        // calling side, given that only one free() cleanup action is required.
        size_t byte_c = obj->pair_c * sizeof(char *);
        for (struct jg_pair const * p = obj->pairs;
            p < obj->pairs + obj->pair_c; p++) {
            byte_c += p->key.byte_c + 1; // null-terminated key string extent
        }
        *keys = calloc(byte_c, 1);
        if (!*keys) {
            return jg->ret = JG_E_CALLOC;
        }
        char * str = (char *) (*keys + obj->pair_c);
        for (size_t i = 0; i < obj->pair_c; i++) {
            (*keys)[i] = str;
            size_t strlen = obj->pairs[i].key.byte_c;
            memcpy(str, obj->pairs[i].key.json, strlen);
            str += strlen + 1; // null_terminate each key string
        }
        *key_c = obj->pair_c;
    }
    return JG_OK;
}

jg_ret jg_root_get_obj(
    jg_t * jg,
    jg_root_obj * opt,
    struct jg_obj const * * v
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_type(jg, &jg->root_in, JG_TYPE_OBJ));
    if (opt) {
        JG_GUARD(handle_obj_options(jg, jg->root_in.obj, opt->keys, opt->key_c,
            opt->min_c_reason, opt->max_c_reason, opt->min_c, opt->max_c));
    }
    *v = jg->root_in.obj;
    return jg->ret = JG_OK;
}

jg_ret jg_arr_get_obj(
    jg_t * jg,
    struct jg_arr const * arr,
    size_t arr_i,
    jg_arr_obj * opt,
    struct jg_obj const * * v
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_arr_index_over(jg, arr, arr_i));
    struct jg_val_in const * child = arr->elems + arr_i;
    JG_GUARD(check_type(jg, child, JG_TYPE_OBJ));
    if (opt) {
        JG_GUARD(handle_obj_options(jg, child->obj, opt->keys, opt->key_c,
            opt->min_c_reason, opt->max_c_reason, opt->min_c, opt->max_c));
    }
    *v = child->obj;
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_obj(
    jg_t * jg,
    struct jg_obj const * obj,
    char const * key,
    jg_obj_obj * opt,
    struct jg_obj const * * v
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * child = NULL;
    JG_GUARD(obj_get_val_by_key(jg, obj, key, true, &child));
    JG_GUARD(check_type(jg, child, JG_TYPE_OBJ));
    if (opt) {
        JG_GUARD(handle_obj_options(jg, child->obj, opt->keys, opt->key_c,
            opt->min_c_reason, opt->max_c_reason, opt->min_c, opt->max_c));
    }
    *v = child->obj;
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_obj_defa(
    jg_t * jg,
    struct jg_obj const * obj,
    char const * key,
    jg_obj_obj_defa * opt,
    struct jg_obj const * * v
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * child = NULL;
    JG_GUARD(obj_get_val_by_key(jg, obj, key, false, &child));
    if (child) {
        JG_GUARD(check_type(jg, child, JG_TYPE_OBJ));
        if (opt) {
            JG_GUARD(handle_obj_options(jg, child->obj, opt->keys, opt->key_c,
                NULL, opt->max_c_reason, 0, opt->max_c));
        }
        *v = child->obj;
    } else {
        static struct jg_obj const empty_obj = {0};
        if (opt) {
            JG_GUARD(handle_obj_options(jg, &empty_obj, opt->keys, opt->key_c,
                NULL, NULL, 0, 0));
        }
        *v = &empty_obj;
    }
    return jg->ret = JG_OK;
}

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_(caller)str() /////////////////////////////////////////

static jg_ret handle_str_size_options(
    jg_t * jg,
    struct jg_val_in const * child,
    size_t dst_byte_c,
    char const * min_byte_c_reason,
    char const * max_byte_c_reason,
    char const * min_cp_c_reason,
    char const * max_cp_c_reason,
    size_t min_byte_c,
    size_t max_byte_c,
    size_t min_cp_c,
    size_t max_cp_c,
    size_t * v_byte_c,
    size_t * v_cp_c
) {
    if (dst_byte_c < min_byte_c) {
        jg->err_val.s = min_byte_c;
        jg->json_cur = child->json;
        set_custom_err_str(jg, min_byte_c_reason);
        return jg->ret = JG_E_GET_STR_BYTE_C_TOO_FEW;
    }
    if (max_byte_c && dst_byte_c > max_byte_c) {
        jg->err_val.s = max_byte_c;
        jg->json_cur = child->json;
        set_custom_err_str(jg, max_byte_c_reason);
        return jg->ret = JG_E_GET_STR_BYTE_C_TOO_MANY;
    }
    if (v_byte_c) {
        *v_byte_c = dst_byte_c;
    }
    size_t cp_c = get_codepoint_c((uint8_t const *) child->json, child->byte_c);
    if (cp_c < min_cp_c) {
        jg->err_val.s = min_cp_c;
        jg->json_cur = child->json;
        set_custom_err_str(jg, min_cp_c_reason);
        return jg->ret = JG_E_GET_STR_CHAR_C_TOO_FEW;
    }
    if (max_cp_c && cp_c > max_cp_c) {
        jg->err_val.s = max_cp_c;
        jg->json_cur = child->json;
        set_custom_err_str(jg, max_cp_c_reason);
        return jg->ret = JG_E_GET_STR_CHAR_C_TOO_MANY;
    }
    if (v_cp_c) {
        *v_cp_c = cp_c;
    }
    return JG_OK;
}

static jg_ret get_str(
    jg_t * jg,
    char const * json_str,
    size_t json_byte_c,
    size_t dst_byte_c,
    bool nullify_empty_str,
    bool omit_null_terminator,
    bool needs_unesc,
    bool needs_alloc,
    char * * v
) {
    if (!json_byte_c && nullify_empty_str) {
        *v = NULL;
        return jg->ret = JG_OK;
    }
    if (needs_alloc) {
        *v = malloc(dst_byte_c + !omit_null_terminator);
        if (!*v) {
            return jg->ret = JG_E_MALLOC;
        }
    } else if (!*v) {
        // Assume caller only wants to know dst_byte_c and/or codepoint_c).
        return jg->ret = JG_OK; // v is NULL, so return without copying.
    }
    if (needs_unesc) {
        json_str_to_unesc_str((uint8_t const *) json_str, json_byte_c,
        (uint8_t *) *v);
    } else {
        memcpy(*v, json_str, json_byte_c);
    }
    if (!omit_null_terminator) {
        (*v)[dst_byte_c] = '\0';
    }
    return jg->ret = JG_OK;
}

static jg_ret root_get_str(
    jg_t * jg,
    jg_root_str * opt,
    bool needs_unesc,
    bool needs_alloc,
    char * * v
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_type(jg, &jg->root_in, JG_TYPE_STR));
    size_t dst_byte_c = needs_unesc ?
        get_unesc_byte_c((uint8_t const *) jg->root_in.json,
        jg->root_in.byte_c) : jg->root_in.byte_c;
    bool nullify_empty_str = false;
    bool omit_null_terminator = false;
    if (opt) {
        JG_GUARD(handle_str_size_options(jg, &jg->root_in, dst_byte_c,
            opt->min_byte_c_reason, opt->max_byte_c_reason,
            opt->min_cp_c_reason, opt->max_cp_c_reason, opt->min_byte_c,
            opt->max_byte_c, opt->min_cp_c, opt->max_cp_c, opt->byte_c,
            opt->codepoint_c));
        nullify_empty_str = opt->nullify_empty_str;
        omit_null_terminator = opt->omit_null_terminator;
    }
    return get_str(jg, jg->root_in.json, jg->root_in.byte_c, dst_byte_c,
        nullify_empty_str, omit_null_terminator, needs_unesc, needs_alloc, v);
}

JG_ROOT_GET(_str, char *) {
    return root_get_str(jg, opt, true, true, v);
}

JG_ROOT_GET(_callerstr, char) {
    return root_get_str(jg, opt, true, false, &v);
}

JG_ROOT_GET(_json_str, char *) {
    return root_get_str(jg, opt, false, true, v);
}

JG_ROOT_GET(_json_callerstr, char) {
    return root_get_str(jg, opt, false, false, &v);
}

static jg_ret arr_get_str(
    jg_t * jg,
    struct jg_arr const * arr,
    size_t arr_i,
    jg_arr_str * opt,
    bool needs_unesc,
    bool needs_alloc,
    char * * v
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_arr_index_over(jg, arr, arr_i));
    struct jg_val_in const * child = arr->elems + arr_i;
    JG_GUARD(check_type(jg, child, JG_TYPE_STR));
    size_t dst_byte_c = needs_unesc ?
        get_unesc_byte_c((uint8_t const *) child->json, child->byte_c) :
        child->byte_c;
    bool nullify_empty_str = false;
    bool omit_null_terminator = false;
    if (opt) {
        JG_GUARD(handle_str_size_options(jg, child, dst_byte_c,
            opt->min_byte_c_reason, opt->max_byte_c_reason,
            opt->min_cp_c_reason, opt->max_cp_c_reason, opt->min_byte_c,
            opt->max_byte_c, opt->min_cp_c, opt->max_cp_c, opt->byte_c,
            opt->codepoint_c));
        nullify_empty_str = opt->nullify_empty_str;
        omit_null_terminator = opt->omit_null_terminator;
    }
    return get_str(jg, child->json, child->byte_c, dst_byte_c,
        nullify_empty_str, omit_null_terminator, needs_unesc, needs_alloc, v);
}

JG_ARR_GET(_str, char *) {
    return arr_get_str(jg, arr, arr_i, opt, true, true, v);
}

JG_ARR_GET(_callerstr, char) {
    return arr_get_str(jg, arr, arr_i, opt, true, false, &v);
}

JG_ARR_GET(_json_str, char *) {
    return arr_get_str(jg, arr, arr_i, opt, false, true, v);
}

JG_ARR_GET(_json_callerstr, char) {
    return arr_get_str(jg, arr, arr_i, opt, false, false, &v);
}

static jg_ret obj_get_str(
    jg_t * jg,
    struct jg_obj const * obj,
    char const * key,
    jg_obj_str * opt,
    bool needs_unesc,
    bool needs_alloc,
    char * * v
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * child = NULL;
    JG_GUARD(obj_get_val_by_key(jg, obj, key, !opt || !opt->defa, &child));
    if (!child) {
        // Use the provided default instead: opt must be non-NULL because
        // otherwise the is_required arg to obj_get_val_by_key() would be true,
        // meaning said function would not return JG_OK when no key was found.
        size_t byte_c = strlen(opt->defa);
        if (opt->byte_c) {
            *opt->byte_c = byte_c;
        }
        if (opt->codepoint_c) {
            *opt->codepoint_c = get_codepoint_c((uint8_t const *) opt->defa,
                byte_c);
        }
        return get_str(jg, opt->defa, byte_c, byte_c, opt->nullify_empty_str,
            opt->omit_null_terminator, false, needs_alloc, v);
    }
    JG_GUARD(check_type(jg, child, JG_TYPE_STR));
    size_t dst_byte_c = needs_unesc ?
        get_unesc_byte_c((uint8_t const *) child->json, child->byte_c) :
        child->byte_c;
    bool nullify_empty_str = false;
    bool omit_null_terminator = false;
    if (opt) {
        JG_GUARD(handle_str_size_options(jg, child, dst_byte_c,
            opt->min_byte_c_reason, opt->max_byte_c_reason,
            opt->min_cp_c_reason, opt->max_cp_c_reason, opt->min_byte_c,
            opt->max_byte_c, opt->min_cp_c, opt->max_cp_c, opt->byte_c,
            opt->codepoint_c));
        nullify_empty_str = opt->nullify_empty_str;
        omit_null_terminator = opt->omit_null_terminator;
    }
    return get_str(jg, child->json, child->byte_c, dst_byte_c,
        nullify_empty_str, omit_null_terminator, needs_unesc, needs_alloc, v);
}

JG_OBJ_GET(_str, char *) {
    return obj_get_str(jg, obj, key, opt, true, true, v);
}

JG_OBJ_GET(_callerstr, char) {
    return obj_get_str(jg, obj, key, opt, true, false, &v);
}

JG_OBJ_GET(_json_str, char *) {
    return obj_get_str(jg, obj, key, opt, false, true, v);
}

JG_OBJ_GET(_json_callerstr, char) {
    return obj_get_str(jg, obj, key, opt, false, false, &v);
}

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_<integer_type>() //////////////////////////////////////

static jg_ret get_signed(
    jg_t * jg,
    char const * str,
    char const * min_reason,
    char const * max_reason,
    intmax_t min,
    intmax_t max,
    intmax_t * i
) {
    // This str is already confirmed to be a valid JSON number by parse_number()
    // in jg_parse.c. Now make sure it's a signed integer number.
    char const * c = str;
    if (*c == '-') {
        c++;
    }
    while (*c >= '0' && *c <= '9') {
        c++;
    }
    if (*c == '.' || *c == 'e' || *c == 'E') {
        return jg->ret = JG_E_GET_NUM_NOT_INTEGER;
    }
    *i = strtoimax(str, NULL, 10);
    if (*i < min || *i == INTMAX_MIN) {
        jg->err_val.i = min;
        jg->json_cur = str;
        set_custom_err_str(jg, min_reason);
        return jg->ret = JG_E_GET_NUM_SIGNED_TOO_SMALL;
    }
    if (*i > max || *i == INTMAX_MAX) {
        jg->err_val.i = max;
        jg->json_cur = str;
        set_custom_err_str(jg, max_reason);
        return jg->ret = JG_E_GET_NUM_SIGNED_TOO_LARGE;
    }
    return JG_OK;
}

static jg_ret get_unsigned(
    jg_t * jg,
    char const * str,
    char const * min_reason,
    char const * max_reason,
    uintmax_t min,
    uintmax_t max,
    uintmax_t * u
) {
    // This str is already confirmed to be a valid JSON number by parse_number()
    // in jg_parse.c. Now make sure it's an unsigned integer number.
    char const * c = str;
    if (*c == '-') {
        return jg->ret = JG_E_GET_NUM_NOT_UNSIGNED;
    }
    while (*c >= '0' && *c <= '9') {
        c++;
    }
    if (*c == '.' || *c == 'e' || *c == 'E') {
        return jg->ret = JG_E_GET_NUM_NOT_INTEGER;
    }
    *u = strtoumax(str, NULL, 10);
    if (*u < min) {
        jg->err_val.i = min;
        jg->json_cur = str;
        set_custom_err_str(jg, min_reason);
        return jg->ret = JG_E_GET_NUM_UNSIGNED_TOO_SMALL;
    }
    if (*u > max || *u == UINTMAX_MAX) {
        jg->err_val.i = max;
        jg->json_cur = str;
        set_custom_err_str(jg, max_reason);
        return jg->ret = JG_E_GET_NUM_UNSIGNED_TOO_LARGE;
    }
    return JG_OK;
}

#define _JG_STR_TO_INT(_max_type, _str_to_int_func, _type_min, _type_max) \
do { \
    _max_type n = 0; \
    if (opt) { \
        JG_GUARD((_str_to_int_func)(jg, str, opt->min_reason, opt->max_reason, \
            opt->min ? *opt->min : (_type_min), \
            opt->max ? *opt->max : (_type_max), &n)); \
    } else { \
        JG_GUARD((_str_to_int_func)(jg, str, NULL, NULL, (_type_min), \
            (_type_max), &n)); \
    } \
    *v = n; \
} while (0)

// This should be a NOP, but nonetheless undefine _ just in case; because
// JG_GET_INT_FUNC prepends it to JG_STR_TO_INT to enable nested expansion of
// _JG_STR_TO_INT.
#undef _

#define JG_GET_FUNC_INT(_suf, _type, _str_to_int) \
/* JG_[ROOT|ARR|OBJ]_GET prototype macros are defined in jgrandson.h */ \
JG_ROOT_GET(_suf, _type) { \
    JG_GUARD(check_state_get(jg)); \
    JG_GUARD(check_type(jg, &jg->root_in, JG_TYPE_NUM)); \
    if (jg->json_over[-1] < '0' || jg->json_over[-1] > '9') { \
        const char * str = jg->root_in.json; \
        _##_str_to_int; \
        return jg->ret = JG_OK; \
    } \
    /* Annoying corner case: given that jg->json_text is not required to be */ \
    /* null-terminated, passing a root-level number type to strto(u)imax() */ \
    /* as-is would be at risk of buffer overflow. */ \
    char * str = malloc(jg->root_in.byte_c + 1); \
    if (!str) { \
        return jg->ret = JG_E_MALLOC; \
    } \
    memcpy(str, jg->root_in.json, jg->root_in.byte_c); \
    str[jg->root_in.byte_c] = '\0'; \
    _##_str_to_int; \
    free(str); \
    return jg->ret = JG_OK; \
} \
\
JG_ARR_GET(_suf, _type) { \
    JG_GUARD(check_state_get(jg)); \
    JG_GUARD(check_arr_index_over(jg, arr, arr_i)); \
    struct jg_val_in const * child = arr->elems + arr_i; \
    JG_GUARD(check_type(jg, child, JG_TYPE_NUM)); \
    const char * str = child->json; \
    _##_str_to_int; \
    return jg->ret = JG_OK; \
} \
\
JG_OBJ_GET(_suf, _type) { \
    JG_GUARD(check_state_get(jg)); \
    struct jg_val_in const * child = NULL; \
    JG_GUARD(obj_get_val_by_key(jg, obj, key, !opt || !opt->defa, &child)); \
    if (!child) { \
        *v = *opt->defa; \
        return jg->ret = JG_OK; \
    } \
    JG_GUARD(check_type(jg, child, JG_TYPE_NUM)); \
    const char * str = child->json; \
    _##_str_to_int; \
    return jg->ret = JG_OK; \
}

#define JG_GET_FUNC_SIGNED(_suf, _type, _type_min, _type_max) \
    JG_GET_FUNC_INT(_suf, _type, \
        JG_STR_TO_INT(intmax_t, get_signed, _type_min, _type_max))

#define JG_GET_FUNC_UNSIGNED(_suf, _type, _type_min, _type_max) \
    JG_GET_FUNC_INT(_suf, _type, \
        JG_STR_TO_INT(uintmax_t, get_unsigned, _type_min, _type_max))

JG_GET_FUNC_SIGNED(_int8, int8_t, INT8_MIN, INT8_MAX)
JG_GET_FUNC_SIGNED(_char, char, CHAR_MIN, CHAR_MAX)
JG_GET_FUNC_SIGNED(_signed_char, signed char, SCHAR_MIN, SCHAR_MAX)
JG_GET_FUNC_SIGNED(_int16, int16_t, INT16_MIN, INT16_MAX)
JG_GET_FUNC_SIGNED(_short, short, SHRT_MIN, SHRT_MAX)
JG_GET_FUNC_SIGNED(_int32, int32_t, INT32_MIN, INT32_MAX)
JG_GET_FUNC_SIGNED(_int, int, INT_MIN, INT_MAX)
JG_GET_FUNC_SIGNED(_int64, int64_t, INT64_MIN, INT64_MAX)
JG_GET_FUNC_SIGNED(_long, long, LONG_MIN, LONG_MAX)
JG_GET_FUNC_SIGNED(_long_long, long long, LLONG_MIN, LLONG_MAX)
JG_GET_FUNC_SIGNED(_intmax, intmax_t, INTMAX_MIN, INTMAX_MAX)

JG_GET_FUNC_UNSIGNED(_uint8, uint8_t, 0, UINT8_MAX)
JG_GET_FUNC_UNSIGNED(_unsigned_char, unsigned char, 0, UCHAR_MAX)
JG_GET_FUNC_UNSIGNED(_uint16, uint16_t, 0, UINT16_MAX)
JG_GET_FUNC_UNSIGNED(_unsigned_short, unsigned short, 0, USHRT_MAX)
JG_GET_FUNC_UNSIGNED(_uint32, uint32_t, 0, UINT32_MAX)
JG_GET_FUNC_UNSIGNED(_unsigned, unsigned, 0, UINT_MAX)
JG_GET_FUNC_UNSIGNED(_uint64, uint64_t, 0, UINT64_MAX)
JG_GET_FUNC_UNSIGNED(_unsigned_long, unsigned long, 0, ULONG_MAX)
JG_GET_FUNC_UNSIGNED(_unsigned_long_long, unsigned long long, 0, ULLONG_MAX)
JG_GET_FUNC_UNSIGNED(_sizet, size_t, 0, SIZE_MAX)
JG_GET_FUNC_UNSIGNED(_uintmax, uintmax_t, 0, UINTMAX_MAX)

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_[float|double|long_double]() //////////////////////////

#define JG_GET_FUNC_FLO(_suf, _type, _str_to_flo_func, _e_out_of_range) \
/* JG_[ROOT|ARR|OBJ]_GET_FLO prototype macros are defined in jgrandson.h */ \
JG_ROOT_GET_FLO(_suf, _type) { \
    JG_GUARD(check_state_get(jg)); \
    JG_GUARD(check_type(jg, &jg->root_in, JG_TYPE_NUM)); \
    if (jg->json_over[-1] < '0' || jg->json_over[-1] > '9') { \
        char * end = NULL; \
        errno = 0; \
        *v = _str_to_flo_func(jg->root_in.json, &end); \
        if (errno) { \
            return jg->ret = _e_out_of_range; \
        } \
        return jg->ret = end < jg->root_in.json + jg->root_in.byte_c ? \
            JG_E_GET_NUM_NOT_FLO : JG_OK; \
    } \
    /* Annoying corner case: given that jg->json_text is not required to be */ \
    /* null-terminated, passing a root-level number type to strto(u)imax() */ \
    /* as-is would be at risk of buffer overflow. */ \
    char * str = malloc(jg->root_in.byte_c + 1); \
    if (!str) { \
        return jg->ret = JG_E_MALLOC; \
    } \
    memcpy(str, jg->root_in.json, jg->root_in.byte_c); \
    str[jg->root_in.byte_c] = '\0'; \
    char * end = NULL; \
    errno = 0; \
    *v = _str_to_flo_func(str, &end); \
    free(str); \
    if (errno) { \
        return jg->ret = _e_out_of_range; \
    } \
    return jg->ret = *end == '\0' ? JG_OK : JG_E_GET_NUM_NOT_FLO; \
} \
\
JG_ARR_GET_FLO(_suf, _type) { \
    JG_GUARD(check_state_get(jg)); \
    JG_GUARD(check_arr_index_over(jg, arr, arr_i)); \
    struct jg_val_in const * child = arr->elems + arr_i; \
    JG_GUARD(check_type(jg, child, JG_TYPE_NUM)); \
    char * end = NULL; \
    errno = 0; \
    *v = _str_to_flo_func(child->json, &end); \
    if (errno) { \
        return jg->ret = _e_out_of_range; \
    } \
    return jg->ret = end < child->json + child->byte_c ? \
        JG_E_GET_NUM_NOT_FLO : JG_OK; \
} \
\
JG_OBJ_GET_FLO(_suf, _type) { \
    JG_GUARD(check_state_get(jg)); \
    struct jg_val_in const * child = NULL; \
    JG_GUARD(obj_get_val_by_key(jg, obj, key, !defa, &child)); \
    if (!child) { \
        *v = *defa; \
        return jg->ret = JG_OK; \
    } \
    char * end = NULL; \
    errno = 0; \
    *v = _str_to_flo_func(child->json, &end); \
    if (errno) { \
        return jg->ret = _e_out_of_range; \
    } \
    return jg->ret = end < child->json + child->byte_c ? \
        JG_E_GET_NUM_NOT_FLO : JG_OK; \
}

JG_GET_FUNC_FLO(_float, float, strtof, JG_E_GET_NUM_FLOAT_OUT_OF_RANGE)
JG_GET_FUNC_FLO(_double, double, strtod, JG_E_GET_NUM_DOUBLE_OUT_OF_RANGE)
JG_GET_FUNC_FLO(_long_double, long double, strtold,
    JG_E_GET_NUM_LONG_DOUBLE_OUT_OF_RANGE)
