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
    struct jg_val_in const * v,
    enum jg_type type
) {
    if (v->type != type) {
        switch (v->type) {
        case JG_TYPE_ARR:
            jg->json_cur = v->arr->json;
            break;
        case JG_TYPE_OBJ:
            jg->json_cur = v->obj->json;
            break;
        default:
            jg->json_cur = v->json;
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

// todo: should actually handle comparing escaped and unescaped key strings ):
static jg_ret obj_get_val_by_key(
    jg_t * jg,
    struct jg_obj const * obj,
    char const * key,
    bool is_required,
    struct jg_val_in const * * val
) {
    // Assumes JG_GUARD(check_src_type(jg, src, JG_TYPE_OBJ)) was already done.
    size_t byte_c = strlen(key);
    for (struct jg_pair const * p = obj->pairs; p < obj->pairs + obj->pair_c;
        p++) {
        // Can't just use strcmp() because parsed strings aren't null-terminated
        if (p->key.byte_c == byte_c && strncmp(p->key.json, key, byte_c)) {
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
    enum jg_type * dst
) {
    JG_GUARD(check_state_get(jg));
    *dst = jg->root_in.type;
    return jg->ret = JG_OK;
}

jg_ret jg_arr_get_json_type(
    jg_t * jg,
    jg_arr_get_t const * src,
    size_t src_i,
    enum jg_type * dst
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_arr_index_over(jg, src, src_i));
    *dst = src->elems[src_i].type;
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_json_type(
    jg_t * jg,
    jg_obj_get_t const * src,
    char const * key,
    enum jg_type * dst
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * v = NULL;
    JG_GUARD(obj_get_val_by_key(jg, src, key, true, &v));
    *dst = v->type;
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
    jg_arr_get_t * * dst,
    size_t * elem_c
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_type(jg, &jg->root_in, JG_TYPE_ARR));
    if (opt) {
        JG_GUARD(handle_arr_options(jg, jg->root_in.arr, opt->min_c_reason,
            opt->max_c_reason, opt->min_c, opt->max_c));
    }
    *dst = jg->root_in.arr;
    *elem_c = jg->root_in.arr->elem_c;
    return jg->ret = JG_OK;
}

typedef struct jg_opt_arr jg_arr_arr;
jg_ret jg_arr_get_arr(
	jg_t * jg,
	jg_arr_get_t const * src,
	size_t src_i,
    jg_arr_arr * opt,
	jg_arr_get_t * * dst,
	size_t * elem_c
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_arr_index_over(jg, src, src_i));
    struct jg_val_in const * v = src->elems + src_i;
    JG_GUARD(check_type(jg, v, JG_TYPE_ARR));
    if (opt) {
        JG_GUARD(handle_arr_options(jg, v->arr, opt->min_c_reason,
            opt->max_c_reason, opt->min_c, opt->max_c));
    }
    *dst = v->arr;
    *elem_c = v->arr->elem_c;
    return jg->ret = JG_OK;
}

typedef struct jg_opt_arr jg_obj_arr;
jg_ret jg_obj_get_arr(
	jg_t * jg,
	jg_obj_get_t const * src,
	char const * key,
    jg_obj_arr * opt,
	jg_arr_get_t * * dst,
	size_t * elem_c
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * v = NULL;
    JG_GUARD(obj_get_val_by_key(jg, src, key, true, &v));
    JG_GUARD(check_type(jg, v, JG_TYPE_ARR));
    if (opt) {
        JG_GUARD(handle_arr_options(jg, v->arr, opt->min_c_reason,
            opt->max_c_reason, opt->min_c, opt->max_c));
    }
    *dst = v->arr;
    *elem_c = v->arr->elem_c;
    return jg->ret = JG_OK;
}

typedef struct jg_opt_arr_defa jg_obj_arr_defa;
jg_ret jg_obj_get_arr_defa(
	jg_t * jg,
	jg_obj_get_t const * src,
	char const * key,
    jg_obj_arr_defa * opt,
	jg_arr_get_t * * dst,
	size_t * elem_c
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * v = NULL;
    JG_GUARD(obj_get_val_by_key(jg, src, key, false, &v));
    if (v) {
        JG_GUARD(check_type(jg, v, JG_TYPE_ARR));
        if (opt) {
            JG_GUARD(handle_arr_options(jg, v->arr, NULL, opt->max_c_reason, 0,
                opt->max_c));
        }
        *dst = v->arr;
        *elem_c = v->arr->elem_c;
    } else {
        static struct jg_arr empty_arr = {0};
        *dst = &empty_arr;
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
	jg_obj_get_t * * dst
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_type(jg, &jg->root_in, JG_TYPE_OBJ));
    if (opt) {
        JG_GUARD(handle_obj_options(jg, jg->root_in.obj, opt->keys, opt->key_c,
            opt->min_c_reason, opt->max_c_reason, opt->min_c, opt->max_c));
    }
    *dst = jg->root_in.obj;
    return jg->ret = JG_OK;
}

jg_ret jg_arr_get_obj(
	jg_t * jg,
	jg_arr_get_t const * src,
	size_t src_i,
    jg_arr_obj * opt,
	jg_obj_get_t * * dst
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_arr_index_over(jg, src, src_i));
    struct jg_val_in const * v = src->elems + src_i;
    JG_GUARD(check_type(jg, v, JG_TYPE_OBJ));
    if (opt) {
        JG_GUARD(handle_obj_options(jg, v->obj, opt->keys, opt->key_c,
            opt->min_c_reason, opt->max_c_reason, opt->min_c, opt->max_c));
    }
    *dst = v->obj;
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_obj(
	jg_t * jg,
	jg_obj_get_t const * src,
	char const * key,
    jg_obj_obj * opt,
	jg_obj_get_t * * dst
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * v = NULL;
    JG_GUARD(obj_get_val_by_key(jg, src, key, true, &v));
    JG_GUARD(check_type(jg, v, JG_TYPE_OBJ));
    if (opt) {
        JG_GUARD(handle_obj_options(jg, v->obj, opt->keys, opt->key_c,
            opt->min_c_reason, opt->max_c_reason, opt->min_c, opt->max_c));
    }
    *dst = v->obj;
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_obj_defa(
	jg_t * jg,
	jg_obj_get_t const * src,
	char const * key,
    jg_obj_obj_defa * opt,
	jg_obj_get_t * * dst
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * v = NULL;
    JG_GUARD(obj_get_val_by_key(jg, src, key, false, &v));
    if (v) {
        JG_GUARD(check_type(jg, v, JG_TYPE_OBJ));
        if (opt) {
            JG_GUARD(handle_obj_options(jg, v->obj, opt->keys, opt->key_c, NULL,
                opt->max_c_reason, 0, opt->max_c));
        }
        *dst = v->obj;
    } else {
        static struct jg_obj const empty_obj = {0};
        if (opt) {
            JG_GUARD(handle_obj_options(jg, &empty_obj, opt->keys, opt->key_c,
                NULL, NULL, 0, 0));
        }
        *dst = &empty_obj;
    }
    return jg->ret = JG_OK;
}

////////////////////////////////////////////////////////////////////////////////
// ..._get_*str*() /////////////////////////////////////////////////////////////

static size_t get_str_char_c(
    char const * str,
    size_t byte_c
) {
    size_t char_c = 0;
    // todo: should actually unescape the string first ):
    for (char const * c = str; c < str + byte_c; c++) {
        char_c += !is_utf8_continuation_byte(*c);
    }
    return char_c;
}

static jg_ret handle_str_size_options(
    jg_t * jg,
    struct jg_val_in const * v,
    char const * min_byte_c_reason,
    char const * max_byte_c_reason,
    char const * min_char_c_reason,
    char const * max_char_c_reason,
    size_t min_byte_c,
    size_t max_byte_c,
    size_t min_char_c,
    size_t max_char_c,
    size_t * dst_byte_c,
    size_t * dst_char_c
) {
    if (v->byte_c < min_byte_c) {
        jg->err_val.s = min_byte_c;
        jg->json_cur = v->json;
        set_custom_err_str(jg, min_byte_c_reason);
        return jg->ret = JG_E_GET_STR_BYTE_C_TOO_FEW;
    }
    if (v->byte_c > max_byte_c) {
        jg->err_val.s = max_byte_c;
        jg->json_cur = v->json;
        set_custom_err_str(jg, max_byte_c_reason);
        return jg->ret = JG_E_GET_STR_BYTE_C_TOO_MANY;
    }
    if (dst_byte_c) {
        *dst_byte_c = v->byte_c;
    }
    size_t char_c = get_str_char_c(v->json, v->byte_c);
    if (char_c < min_char_c) {
        jg->err_val.s = min_char_c;
        jg->json_cur = v->json;
        set_custom_err_str(jg, min_char_c_reason);
        return jg->ret = JG_E_GET_STR_CHAR_C_TOO_FEW;
    }
    if (char_c > max_char_c) {
        jg->err_val.s = max_char_c;
        jg->json_cur = v->json;
        set_custom_err_str(jg, max_char_c_reason);
        return jg->ret = JG_E_GET_STR_CHAR_C_TOO_MANY;
    }
    if (dst_char_c) {
        *dst_char_c = char_c;
    }
    return JG_OK;
}

static jg_ret get_str(
    jg_t * jg,
    char const * str,
    size_t byte_c,
    bool nullify_empty_str,
    bool omit_null_terminator,
    bool needs_alloc,
    char * * dst
) {
     if (!byte_c && nullify_empty_str) {
        *dst = NULL;
        return jg->ret = JG_OK;
    }
    if (needs_alloc) {
        *dst = malloc(byte_c + !omit_null_terminator);
        if (!*dst) {
            return jg->ret = JG_E_MALLOC;
        }
    } else if (!*dst) {
        // Assume .._get_str() caller only wants to know byte_c (or char_c).
        return jg->ret = JG_OK; // dst is NULL, so return without copying.
    }
    memcpy(*dst, str, byte_c);
    if (!omit_null_terminator) {
        (*dst)[byte_c] = '\0';
    }
    return jg->ret = JG_OK;
}

static jg_ret root_get_str(
    jg_t * jg,
    jg_root_str * opt,
    bool needs_alloc,
    char * * dst
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_type(jg, &jg->root_in, JG_TYPE_STR));
    bool nullify_empty_str = false;
    bool omit_null_terminator = false;
    if (opt) {
        JG_GUARD(handle_str_size_options(jg, &jg->root_in,
            opt->min_byte_c_reason, opt->max_byte_c_reason,
            opt->min_char_c_reason, opt->max_char_c_reason, opt->min_byte_c,
            opt->max_byte_c, opt->min_char_c, opt->max_char_c, opt->byte_c,
            opt->char_c));
        nullify_empty_str = opt->nullify_empty_str;
        omit_null_terminator = opt->omit_null_terminator;
    }
    return get_str(jg, jg->root_in.json, jg->root_in.byte_c, nullify_empty_str,
        omit_null_terminator, needs_alloc, dst);
}

jg_ret jg_root_get_str(
    jg_t * jg,
    jg_root_str * opt,
    char * dst
) {
    return root_get_str(jg, opt, false, &dst);
}

jg_ret jg_root_get_astr(
    jg_t * jg,
    jg_root_astr * opt,
    char * * dst
) {
    return root_get_str(jg, opt, true, dst);
}

static jg_ret arr_get_str(
    jg_t * jg,
    jg_arr_get_t const * src,
    size_t src_i,
    jg_arr_str * opt,
    bool needs_alloc,
    char * * dst
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_arr_index_over(jg, src, src_i));
    struct jg_val_in const * v = src->elems + src_i;
    JG_GUARD(check_type(jg, v, JG_TYPE_STR));
    bool nullify_empty_str = false;
    bool omit_null_terminator = false;
    if (opt) {
        JG_GUARD(handle_str_size_options(jg, v, opt->min_byte_c_reason,
            opt->max_byte_c_reason, opt->min_char_c_reason,
            opt->max_char_c_reason, opt->min_byte_c, opt->max_byte_c,
            opt->min_char_c, opt->max_char_c, opt->byte_c, opt->char_c));
        nullify_empty_str = opt->nullify_empty_str;
        omit_null_terminator = opt->omit_null_terminator;
    }
    return get_str(jg, v->json, v->byte_c, nullify_empty_str,
        omit_null_terminator, needs_alloc, dst);
}

jg_ret jg_arr_get_str(
    jg_t * jg,
    jg_arr_get_t const * src,
    size_t src_i,
    jg_arr_str * opt,
    char * dst
) {
    return arr_get_str(jg, src, src_i, opt, false, &dst);
}

jg_ret jg_arr_get_astr(
    jg_t * jg,
    jg_arr_get_t const * src,
    size_t src_i,
    jg_arr_astr * opt,
    char * * dst
) {
    return arr_get_str(jg, src, src_i, opt, true, dst);
}

static jg_ret obj_get_str(
    jg_t * jg,
    jg_obj_get_t const * src,
    char const * key,
    jg_obj_str * opt,
    bool needs_alloc,
    char * * dst
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * v = NULL;
    JG_GUARD(obj_get_val_by_key(jg, src, key, !opt || !opt->defa, &v));
    if (!v) {
        // Use the provided default instead: opt must be non-NULL because
        // otherwise the is_required arg to obj_get_val_by_key() would be true,
        // meaning said function would not return JG_OK when no key was found.
        size_t byte_c = strlen(opt->defa);
        if (opt->byte_c) {
            *opt->byte_c = byte_c;
        }
        if (opt->char_c) {
            *opt->char_c = get_str_char_c(opt->defa, byte_c);
        }
        return get_str(jg, opt->defa, byte_c, opt->nullify_empty_str,
            opt->omit_null_terminator, needs_alloc, dst);
    }
    JG_GUARD(check_type(jg, v, JG_TYPE_STR));
    bool nullify_empty_str = false;
    bool omit_null_terminator = false;
    if (opt) {
        JG_GUARD(handle_str_size_options(jg, v, opt->min_byte_c_reason,
            opt->max_byte_c_reason, opt->min_char_c_reason,
            opt->max_char_c_reason, opt->min_byte_c, opt->max_byte_c,
            opt->min_char_c, opt->max_char_c, opt->byte_c, opt->char_c));
        nullify_empty_str = opt->nullify_empty_str;
        omit_null_terminator = opt->omit_null_terminator;
    }
    return get_str(jg, v->json, v->byte_c, nullify_empty_str,
        omit_null_terminator, needs_alloc, dst);
}

jg_ret jg_obj_get_str(
    jg_t * jg,
    jg_obj_get_t const * src,
    char const * key,
    jg_obj_str * opt,
    char * dst
) {
    return obj_get_str(jg, src, key, opt, false, &dst);
}

jg_ret jg_obj_get_astr(
    jg_t * jg,
    jg_obj_get_t const * src,
    char const * key,
    jg_obj_astr * opt,
    char * * dst
) {
    return obj_get_str(jg, src, key, opt, true, dst);
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
    jg_arr_get_t const * src,
    size_t src_i
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_arr_index_over(jg, src, src_i));
    JG_GUARD(check_type(jg, src->elems + src_i, JG_TYPE_NULL));
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_null(
    jg_t * jg,
    jg_obj_get_t const * src,
    char const * key
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * v = NULL;
    JG_GUARD(obj_get_val_by_key(jg, src, key, true, &v));
    JG_GUARD(check_type(jg, v, JG_TYPE_NULL));
    return jg->ret = JG_OK;
}

////////////////////////////////////////////////////////////////////////////////
// jg_[root|arr|obj]_get_bool() ////////////////////////////////////////////////

jg_ret jg_root_get_bool(
    jg_t * jg,
    bool * dst
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_type(jg, &jg->root_in, JG_TYPE_BOOL));
    *dst = jg->root_in.bool_is_true;
    return jg->ret = JG_OK;
}

jg_ret jg_arr_get_bool(
    jg_t * jg,
    jg_arr_get_t const * src,
    size_t src_i,
    bool * dst
) {
    JG_GUARD(check_state_get(jg));
    JG_GUARD(check_arr_index_over(jg, src, src_i));
    struct jg_val_in const * v = src->elems + src_i;
    JG_GUARD(check_type(jg, v, JG_TYPE_BOOL));
    *dst = v->bool_is_true;
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_bool(
    jg_t * jg,
    jg_obj_get_t const * src,
    char const * key,
    bool const * defa,
    bool * dst
) {
    JG_GUARD(check_state_get(jg));
    struct jg_val_in const * v = NULL;
    JG_GUARD(obj_get_val_by_key(jg, src, key, !defa, &v));
    if (v) {
        JG_GUARD(check_type(jg, v, JG_TYPE_BOOL));
        *dst = v->bool_is_true;
    } else {
        *dst = *defa;
    }
    return jg->ret = JG_OK;
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
            opt->min ? opt->min : (_type_min), \
            opt->max ? opt->max : (_type_max), &n)); \
    } else { \
        JG_GUARD((_str_to_int_func)(jg, str, NULL, NULL, (_type_min), \
            (_type_max), &n)); \
    } \
    *dst = n; \
} while (0)

// This should be a NOP, but nonetheless undefine _ just in case; because
// JG_GET_INT_FUNC prepends it to JG_STR_TO_INT to enable nested expansion of
// _JG_STR_TO_INT.
#undef _

#define JG_GET_FUNC_INT(_suf, _type, _str_to_int) \
/* JG_[ROOT|ARR|OBJ]_GET_INT prototype macros are defined in jgrandson.h */ \
JG_ROOT_GET_INT(_suf, _type) { \
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
JG_ARR_GET_INT(_suf, _type) { \
    JG_GUARD(check_state_get(jg)); \
    JG_GUARD(check_arr_index_over(jg, src, src_i)); \
    struct jg_val_in const * v = src->elems + src_i; \
    JG_GUARD(check_type(jg, v, JG_TYPE_NUM)); \
    const char * str = v->json; \
    _##_str_to_int; \
    return jg->ret = JG_OK; \
} \
\
JG_OBJ_GET_INT(_suf, _type) { \
    JG_GUARD(check_state_get(jg)); \
    struct jg_val_in const * v = NULL; \
    JG_GUARD(obj_get_val_by_key(jg, src, key, !opt || !opt->defa, &v)); \
    if (!v) { \
        *dst = *opt->defa; \
        return jg->ret = JG_OK; \
    } \
    JG_GUARD(check_type(jg, v, JG_TYPE_NUM)); \
    const char * str = v->json; \
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
        *dst = _str_to_flo_func(jg->root_in.json, &end); \
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
    *dst = _str_to_flo_func(str, &end); \
    free(str); \
    if (errno) { \
        return jg->ret = _e_out_of_range; \
    } \
    return jg->ret = *end == '\0' ? JG_OK : JG_E_GET_NUM_NOT_FLO; \
} \
\
JG_ARR_GET_FLO(_suf, _type) { \
    JG_GUARD(check_state_get(jg)); \
    JG_GUARD(check_arr_index_over(jg, src, src_i)); \
    struct jg_val_in const * v = src->elems + src_i; \
    JG_GUARD(check_type(jg, v, JG_TYPE_NUM)); \
    char * end = NULL; \
    errno = 0; \
    *dst = _str_to_flo_func(v->json, &end); \
    if (errno) { \
        return jg->ret = _e_out_of_range; \
    } \
    return jg->ret = end < v->json + v->byte_c ? JG_E_GET_NUM_NOT_FLO : JG_OK; \
} \
\
JG_OBJ_GET_FLO(_suf, _type) { \
    JG_GUARD(check_state_get(jg)); \
    struct jg_val_in const * v = NULL; \
    JG_GUARD(obj_get_val_by_key(jg, src, key, !defa, &v)); \
    if (!v) { \
        *dst = *defa; \
        return jg->ret = JG_OK; \
    } \
    char * end = NULL; \
    errno = 0; \
    *dst = _str_to_flo_func(v->json, &end); \
    if (errno) { \
        return jg->ret = _e_out_of_range; \
    } \
    return jg->ret = end < v->json + v->byte_c ? JG_E_GET_NUM_NOT_FLO : JG_OK; \
}

JG_GET_FUNC_FLO(_float, float, strtof, JG_E_GET_NUM_FLOAT_OUT_OF_RANGE)
JG_GET_FUNC_FLO(_double, double, strtod, JG_E_GET_NUM_DOUBLE_OUT_OF_RANGE)
JG_GET_FUNC_FLO(_long_double, long double, strtold,
    JG_E_GET_NUM_LONG_DOUBLE_OUT_OF_RANGE)
