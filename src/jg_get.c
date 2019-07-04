#include "jgrandson_internal.h"

static jg_ret check_type(
    jg_t * jg,
    struct jg_val const * v,
    enum jg_type type,
    jg_ret ret
) {
    if (v->type != type) {
        jg->err_expected.type = type;
        jg->err_received.type = v->type;
        return jg->ret = ret;
    }
    return JG_OK;
}

static jg_ret check_src_type(
    jg_t * jg,
    struct jg_val const * v,
    enum jg_type type
) {
    return check_type(jg, v, type, JG_E_GET_WRONG_SRC_TYPE);
}

static jg_ret check_dst_type(
    jg_t * jg,
    struct jg_val const * v,
    enum jg_type type
) {
    return check_type(jg, v, type, JG_E_GET_WRONG_DST_TYPE);
}

static jg_ret check_src_arr_over(
    jg_t * jg,
    struct jg_val const * arr,
    size_t arr_i
) {
    if (arr->elem_c <= arr_i) {
        jg->err_expected.s = arr_i;
        jg->err_received.s = arr->elem_c;
        return jg->ret = JG_E_GET_SRC_ARR_OVER;
    }
    return JG_OK;
}

static struct jg_val * obj_get_val_by_key(
    struct jg_val const * v,
    char const * key
) {
    // Assumes JG_GUARD(check_src_type(jg, src, JG_TYPE_OBJ)) was already done.
    size_t byte_c = strlen(key);
    for (struct jg_keyval * kv = v->obj; kv < v->obj + v->keyval_c; kv++) {
        // Can't just use strcmp() because jg_val strings aren't null-terminated
        if (kv->key.byte_c == byte_c &&
            !strncmp((char const *) kv->key.str, key, byte_c)) {
            return &kv->val;
        }
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// ..._get_json_type() /////////////////////////////////////////////////////////

jg_ret jg_root_get_json_type(jg_t * jg,
    enum jg_type * dst
) {
    JG_GUARD(check_state(jg, JG_STATE_GET));
    *dst = jg->root_val.type;
    return jg->ret = JG_OK;
}

jg_ret jg_arr_get_json_type(jg_t * jg, jg_arr_t const * src, size_t src_i,
    enum jg_type * dst
) {
    JG_GUARD(check_state(jg, JG_STATE_GET));
    JG_GUARD(check_src_type(jg, src, JG_TYPE_ARR));
    JG_GUARD(check_src_arr_over(jg, src, src_i));
    *dst = src->arr[src_i].type;
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_json_type(jg_t * jg, jg_obj_t const * src, char const * src_k,
    enum jg_type * dst
) {
    JG_GUARD(check_state(jg, JG_STATE_GET));
    JG_GUARD(check_src_type(jg, src, JG_TYPE_OBJ));
    struct jg_val * v = obj_get_val_by_key(src, src_k);
    if (!v) return jg->ret = JG_E_GET_KEY_NOT_FOUND;
    *dst = v->type;
    return jg->ret = JG_OK;
}

////////////////////////////////////////////////////////////////////////////////
// ..._get_arr...() ////////////////////////////////////////////////////////////

static jg_ret check_dst_arr_elem_c(
    jg_t * jg,
    struct jg_val * v,
    size_t min_c,
    size_t max_c
) {
    if (v->elem_c < min_c) {
        jg->err_expected.s = min_c;
        jg->err_received.s = v->elem_c;
        return jg->ret = JG_E_GET_ARR_TOO_SHORT;
    }
    if (max_c && v->elem_c > max_c) {
        jg->err_expected.s = max_c;
        jg->err_received.s = v->elem_c;
        return jg->ret = JG_E_GET_ARR_TOO_LONG;
    }
    return JG_OK;
}

jg_ret jg_root_get_arr(jg_t * jg,
    size_t min_c, size_t max_c, jg_arr_t * * dst, size_t * elem_c
) {
    JG_GUARD(check_state(jg, JG_STATE_GET));
    JG_GUARD(check_dst_type(jg, &jg->root_val, JG_TYPE_ARR));
    JG_GUARD(check_dst_arr_elem_c(jg, &jg->root_val, min_c, max_c));
    *dst = &jg->root_val;
    *elem_c = jg->root_val.elem_c;
    return jg->ret = JG_OK;
}

jg_ret jg_arr_get_arr(jg_t * jg, jg_arr_t const * src, size_t src_i,
    size_t min_c, size_t max_c, jg_arr_t * * dst, size_t * elem_c
) {
    JG_GUARD(check_state(jg, JG_STATE_GET));
    JG_GUARD(check_src_type(jg, src, JG_TYPE_ARR));
    JG_GUARD(check_src_arr_over(jg, src, src_i));
    struct jg_val * v = src->arr + src_i;
    JG_GUARD(check_dst_type(jg, v, JG_TYPE_ARR));
    JG_GUARD(check_dst_arr_elem_c(jg, v, min_c, max_c));
    *dst = v;
    *elem_c = v->elem_c;
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_arr(jg_t * jg, jg_obj_t const * src, char const * src_k,
    size_t min_c, size_t max_c, jg_arr_t * * dst, size_t * elem_c
) {
    JG_GUARD(check_state(jg, JG_STATE_GET));
    JG_GUARD(check_src_type(jg, src, JG_TYPE_OBJ));
    struct jg_val * v = obj_get_val_by_key(src, src_k);
    if (!v) return jg->ret = JG_E_GET_KEY_NOT_FOUND;
    JG_GUARD(check_dst_type(jg, v, JG_TYPE_ARR));
    JG_GUARD(check_dst_arr_elem_c(jg, v, min_c, max_c));
    *dst = v;
    *elem_c = v->elem_c;
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_arr_defa(jg_t * jg, jg_obj_t const * src, char const * src_k,
    size_t max_c, jg_arr_t * * dst, size_t * elem_c
) {
    JG_GUARD(check_state(jg, JG_STATE_GET));
    JG_GUARD(check_src_type(jg, src, JG_TYPE_OBJ));
    struct jg_val * v = obj_get_val_by_key(src, src_k);
    if (v) {
        JG_GUARD(check_dst_type(jg, v, JG_TYPE_ARR));
        JG_GUARD(check_dst_arr_elem_c(jg, v, 0, max_c));
        *dst = v;
        *elem_c = v->elem_c;
    } else {
        static struct jg_val const empty_arr = {.type = JG_TYPE_ARR};
        *dst = &empty_arr;
        *elem_c = 0;
    }
    return jg->ret = JG_OK;
}

////////////////////////////////////////////////////////////////////////////////
// ..._get_obj...() ////////////////////////////////////////////////////////////

jg_ret jg_root_get_obj(jg_t * jg,
    jg_obj_t * * dst
) {
    JG_GUARD(check_state(jg, JG_STATE_GET));
    JG_GUARD(check_dst_type(jg, &jg->root_val, JG_TYPE_OBJ));
    *dst = &jg->root_val;
    return jg->ret = JG_OK;
}

jg_ret jg_arr_get_obj(jg_t * jg, jg_arr_t const * src, size_t src_i,
    jg_obj_t * * dst
) {
    JG_GUARD(check_state(jg, JG_STATE_GET));
    JG_GUARD(check_src_type(jg, src, JG_TYPE_ARR));
    JG_GUARD(check_src_arr_over(jg, src, src_i));
    struct jg_val * v = src->arr + src_i;
    JG_GUARD(check_dst_type(jg, v, JG_TYPE_OBJ));
    *dst = v;
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_obj(jg_t * jg, jg_obj_t const * src, char const * src_k,
    jg_obj_t * * dst
) {
    JG_GUARD(check_state(jg, JG_STATE_GET));
    JG_GUARD(check_src_type(jg, src, JG_TYPE_OBJ));
    struct jg_val * v = obj_get_val_by_key(src, src_k);
    if (!v) return jg->ret = JG_E_GET_KEY_NOT_FOUND;
    JG_GUARD(check_dst_type(jg, v, JG_TYPE_OBJ));
    *dst = v;
    return jg->ret = JG_OK;
}

jg_ret jg_obj_get_obj_defa(jg_t * jg, jg_obj_t const * src, char const * src_k,
    jg_obj_t * * dst
) {
    JG_GUARD(check_state(jg, JG_STATE_GET));
    JG_GUARD(check_src_type(jg, src, JG_TYPE_OBJ));
    struct jg_val * v = obj_get_val_by_key(src, src_k);
    if (v) {
        JG_GUARD(check_dst_type(jg, v, JG_TYPE_OBJ));
        *dst = v;
    } else {
        static struct jg_val const empty_obj = {.type = JG_TYPE_OBJ};
        *dst = &empty_obj;
    }
    return jg->ret = JG_OK;
}

////////////////////////////////////////////////////////////////////////////////
// jg_obj_get_keys() ///////////////////////////////////////////////////////////

// Allocate a single heap buffer and fill it with pointers to each key string
// copy, followed by said key strings themselves (i.e., have the buffer header
// point to string locations within the tail of the same buffer). Aside from
// being memory efficient, this causes the least work for the calling side,
// given that only one free() cleanup action is required of it.
jg_ret jg_obj_get_keys(jg_t * jg, jg_obj_t * v,
    char * * * dst, size_t * key_c
) {
    JG_GUARD(check_state(jg, JG_STATE_GET));
    JG_GUARD(check_src_type(jg, v, JG_TYPE_OBJ));
    size_t byte_c = v->keyval_c * sizeof(char *);
    for (struct jg_keyval * kv = v->obj; kv < v->obj + v->keyval_c; kv++) {
        byte_c += kv->key.byte_c + 1; // null-terminated key string space
    }
    *dst = calloc(byte_c, 1);
    if (!*dst) return jg->ret = JG_E_CALLOC;
    char * str = (char *) (*dst + v->keyval_c);
    for (size_t i = 0; i < v->keyval_c; i++) {
        (*dst)[i] = str;
        size_t strlen = v->obj[i].key.byte_c;
        memcpy(str, v->obj[i].key.str, strlen);
        str += strlen + 1; // null_terminate each key string
    }
    *key_c = v->keyval_c;
    return jg->ret = JG_OK;
}

////////////////////////////////////////////////////////////////////////////////
// ..._get_*str*() /////////////////////////////////////////////////////////////

static jg_ret get_str(
    jg_t * jg,
    struct jg_val * v,
    uint8_t * * dst,
    jg_opt_str * opt,
    bool needs_alloc
) {
    *opt->byte_c = v->byte_c;
    if (v->byte_c < opt->min_byte_c) {
        jg->err_expected.s = opt->min_byte_c;
        jg->err_received.s = v->byte_c;
        if (opt->min_byte_c_estr) {
            free_err_str(jg);
            jg->err_str = opt->min_byte_c_estr;
            return jg->ret = JG_E_GET_STR_CUSTOM;
        }
        return jg->ret = JG_E_GET_STR_BYTE_C_TOO_FEW;
    }
    if (v->byte_c > opt->max_byte_c) {
        jg->err_expected.s = opt->max_byte_c;
        jg->err_received.s = v->byte_c;
        if (opt->max_byte_c_estr) {
            free_err_str(jg);
            jg->err_str = opt->max_byte_c_estr;
            return jg->ret = JG_E_GET_STR_CUSTOM;
        }
        return jg->ret = JG_E_GET_STR_BYTE_C_TOO_MANY;
    }
    size_t char_c = 0;
    for (uint8_t const * u = v->str; u < v->str + v->byte_c; u++) {
        char_c += JG_IS_1ST_UTF8_BYTE(*u);
    }
    *opt->char_c = char_c;
    if (char_c < opt->min_char_c) {
        jg->err_expected.s = opt->min_char_c;
        jg->err_received.s = char_c;
        if (opt->min_char_c_estr) {
            free_err_str(jg);
            jg->err_str = opt->min_char_c_estr;
            return jg->ret = JG_E_GET_STR_CUSTOM;
        }
        return jg->ret = JG_E_GET_STR_CHAR_C_TOO_FEW;
    }
    if (char_c > opt->max_char_c) {
        jg->err_expected.s = opt->max_char_c;
        jg->err_received.s = char_c;
        if (opt->max_char_c_estr) {
            free_err_str(jg);
            jg->err_str = opt->max_char_c_estr;
            return jg->ret = JG_E_GET_STR_CUSTOM;
        }
        return jg->ret = JG_E_GET_STR_CHAR_C_TOO_MANY;
    }
    if (!v->byte_c && opt->nullify_empty_str) {
        *dst = NULL;
        return jg->ret = JG_OK;
    }
    if (needs_alloc) {
        *dst = malloc(v->byte_c + !opt->omit_null_terminator);
        if (!dst) return jg->ret = JG_E_MALLOC;
    }
    memcpy(*dst, v->str, v->byte_c);
    if (!opt->omit_null_terminator) (*dst)[v->byte_c] = '\0';
    return jg->ret = JG_OK;
}

static jg_ret root_get_str(jg_t * jg,
    uint8_t * * dst, jg_opt_str * opt, bool needs_alloc
) {
    JG_GUARD(check_state(jg, JG_STATE_GET));
    JG_GUARD(check_dst_type(jg, &jg->root_val, JG_TYPE_STR));
    return get_str(jg, &jg->root_val, dst, opt, needs_alloc);
}

jg_ret jg_root_get_str(jg_t * jg,
    char * dst, jg_opt_str * opt
) {
    return root_get_str(jg, (uint8_t * *) &dst, opt, false);
}

jg_ret jg_root_get_stra(jg_t * jg,
    char * * dst, jg_opt_str * opt
) {
    return root_get_str(jg, (uint8_t * *) dst, opt, true);
}

jg_ret jg_root_get_ustr(jg_t * jg,
    uint8_t * dst, jg_opt_str * opt
) {
    return root_get_str(jg, &dst, opt, false);
}

jg_ret jg_root_get_ustra(jg_t * jg,
    uint8_t * * dst, jg_opt_str * opt
) {
    return root_get_str(jg, dst, opt, true);
}

static jg_ret arr_get_str(jg_t * jg, jg_arr_t const * src, size_t src_i,
    uint8_t * * dst, jg_opt_str * opt, bool needs_alloc
) {
    JG_GUARD(check_state(jg, JG_STATE_GET));
    JG_GUARD(check_src_type(jg, src, JG_TYPE_ARR));
    JG_GUARD(check_src_arr_over(jg, src, src_i));
    struct jg_val * v = src->arr + src_i;
    JG_GUARD(check_dst_type(jg, v, JG_TYPE_STR));
    return get_str(jg, v, dst, opt, needs_alloc);
}

jg_ret jg_arr_get_str(jg_t * jg, jg_arr_t const * src, size_t src_i,
    char * dst, jg_opt_str * opt
) {
    return arr_get_str(jg, src, src_i, (uint8_t * *) &dst, opt, false);
}

jg_ret jg_arr_get_stra(jg_t * jg, jg_arr_t const * src, size_t src_i,
    char * * dst, jg_opt_str * opt
) {
    return arr_get_str(jg, src, src_i, (uint8_t * *) dst, opt, true);
}

jg_ret jg_arr_get_ustr(jg_t * jg, jg_arr_t const * src, size_t src_i,
    uint8_t * dst, jg_opt_str * opt
) {
    return arr_get_str(jg, src, src_i, &dst, opt, false);
}

jg_ret jg_arr_get_ustra(jg_t * jg, jg_arr_t const * src, size_t src_i,
    uint8_t * * dst, jg_opt_str * opt
) {
    return arr_get_str(jg, src, src_i, dst, opt, true);
}

static jg_ret obj_get_str(jg_t * jg, jg_obj_t const * src, char const * src_k,
    uint8_t const * defa, uint8_t * * dst, jg_opt_str * opt, bool needs_alloc
) {
    JG_GUARD(check_state(jg, JG_STATE_GET));
    JG_GUARD(check_src_type(jg, src, JG_TYPE_OBJ));
    struct jg_val * v = obj_get_val_by_key(src, src_k);
    if (v) {
        JG_GUARD(check_dst_type(jg, v, JG_TYPE_STR));
        return get_str(jg, v, dst, opt, needs_alloc);
    }
    return defa ? get_str(jg, &(struct jg_val){
        .str = defa,
        .byte_c = strlen((char *) defa)
    }, dst, opt, needs_alloc) : (jg->ret = JG_E_GET_KEY_NOT_FOUND);
}

jg_ret jg_obj_get_str(jg_t * jg, jg_obj_t const * src, char const * src_k,
    char const * defa, char * dst, jg_opt_str * opt
) {
    return obj_get_str(jg, src, src_k, (uint8_t const *) defa,
        (uint8_t * *) &dst, opt, false);
}

jg_ret jg_obj_get_stra(jg_t * jg, jg_obj_t const * src, char const * src_k,
    char const * defa, char * * dst, jg_opt_str * opt
) {
    return obj_get_str(jg, src, src_k, (uint8_t const *) defa,
        (uint8_t * *) dst, opt, true);
}

jg_ret jg_obj_get_ustr(jg_t * jg, jg_obj_t const * src, char const * src_k,
    uint8_t const * defa, uint8_t * dst, jg_opt_str * opt
) {
    return obj_get_str(jg, src, src_k, defa, &dst, opt, false);
}

jg_ret jg_obj_get_ustra(jg_t * jg, jg_obj_t const * src, char const * src_k,
    uint8_t const * defa, uint8_t * * dst, jg_opt_str * opt
) {
    return obj_get_str(jg, src, src_k, defa, dst, opt, true);
}
