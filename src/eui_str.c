#include "eui/eui_str.h"
#include "eui/eui_allocator.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <ctype.h>

/* ---- internal helpers ---- */

static void reserve(eui_str_t *s, size_t need) {
    /* need includes '\0' */
    if (need <= s->cap) return;

    size_t min_cap = need + 16; /* extra breathing room */
    size_t new_cap  = s->cap ? (s->cap * 2) : min_cap;
    if (new_cap < min_cap) new_cap = min_cap;

    char *new_data = (char*)eui_malloc(new_cap);
    assert(new_data);

    if (s->data && s->len > 0) {
        memcpy(new_data, s->data, s->len + 1); /* +1 for '\0' */
    } else if (s->data) {
        new_data[0] = '\0';
    }

    if (s->data) eui_free(s->data);
    s->data = new_data;
    s->cap  = new_cap;
}

/* ---- lifecycle ---- */

void eui_str_init(eui_str_t *s) {
    s->data = NULL;
    s->len  = 0;
    s->cap  = 0;
}

void eui_str_clear(eui_str_t *s) {
    if (s->data) {
        eui_free(s->data);
        s->data = NULL;
    }
    s->len = 0;
    s->cap = 0;
}

eui_str_t* eui_str_create(void) {
    eui_str_t *s = (eui_str_t*)eui_malloc(sizeof(eui_str_t));
    assert(s);
    s->data = NULL;
    s->len  = 0;
    s->cap  = 0;
    return s;
}

void eui_str_destroy(eui_str_t *s) {
    if (!s) return;
    if (s->data) eui_free(s->data);
    eui_free(s);
}

/* ---- assignment ---- */

void eui_str_set(eui_str_t *s, const char *src) {
    if (!src) src = "";
    size_t src_len = strlen(src);
    size_t need    = src_len + 1;
    reserve(s, need);
    memcpy(s->data, src, need);
    s->len = src_len;
}

void eui_str_copy(eui_str_t *dst, const eui_str_t *src) {
    if (!src->data) {
        eui_str_set(dst, "");
        return;
    }
    size_t need = src->len + 1;
    reserve(dst, need);
    memcpy(dst->data, src->data, need);
    dst->len = src->len;
}

/* ---- append ---- */

void eui_str_append(eui_str_t *s, const char *src) {
    if (!src || !*src) return;
    size_t add_len = strlen(src);
    size_t new_len = s->len + add_len;
    reserve(s, new_len + 1);
    memcpy(s->data + s->len, src, add_len + 1); /* +1 for '\0' */
    s->len = new_len;
}

void eui_str_append_str(eui_str_t *s, const eui_str_t *src) {
    if (!src->data || src->len == 0) return;
    size_t new_len = s->len + src->len;
    reserve(s, new_len + 1);
    memcpy(s->data + s->len, src->data, src->len + 1);
    s->len = new_len;
}

void eui_str_append_char(eui_str_t *s, char c) {
    size_t new_len = s->len + 1;
    reserve(s, new_len + 1);
    s->data[s->len]     = c;
    s->data[new_len]    = '\0';
    s->len              = new_len;
}

/* ---- printf ---- */

static void vprintf_into(eui_str_t *s, bool append, const char *fmt, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);
    int need = vsnprintf(NULL, 0, fmt, args_copy);
    va_end(args_copy);
    assert(need >= 0);

    size_t old_len = append ? s->len : 0;
    size_t new_len = old_len + (size_t)need;
    reserve(s, new_len + 1);
    vsnprintf(s->data + old_len, (size_t)need + 1, fmt, args);
    s->len = new_len;
}

void eui_str_printf(eui_str_t *s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf_into(s, false, fmt, args);
    va_end(args);
}

void eui_str_append_printf(eui_str_t *s, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf_into(s, true, fmt, args);
    va_end(args);
}

/* ---- access ---- */

const char* eui_str_cstr(const eui_str_t *s) {
    static const char empty[1] = "";
    return s->data ? s->data : empty;
}

size_t eui_str_len(const eui_str_t *s) {
    return s->len;
}

bool eui_str_empty(const eui_str_t *s) {
    return s->len == 0;
}

/* ---- compare ---- */

bool eui_str_equals(const eui_str_t *a, const char *b) {
    if (!b) return a->len == 0;
    if (!a->data) return *b == '\0';
    return strcmp(a->data, b) == 0;
}

bool eui_str_equals_str(const eui_str_t *a, const eui_str_t *b) {
    if (a->len != b->len) return false;
    if (a->len == 0) return true;
    return memcmp(a->data, b->data, a->len) == 0;
}

int eui_str_cmp(const eui_str_t *a, const eui_str_t *b) {
    if (!a->data && !b->data) return 0;
    if (!a->data) return -1;
    if (!b->data) return 1;
    return strcmp(a->data, b->data);
}

/* ---- trim ---- */

void eui_str_trim_left(eui_str_t *s) {
    if (!s->data || s->len == 0) return;
    const char *p = s->data;
    while (*p && isspace((unsigned char)*p)) p++;
    size_t skip = (size_t)(p - s->data);
    if (skip == 0) return;
    size_t new_len = s->len - skip;
    memmove(s->data, s->data + skip, new_len + 1);
    s->len = new_len;
}

void eui_str_trim_right(eui_str_t *s) {
    if (!s->data || s->len == 0) return;
    size_t i = s->len;
    while (i > 0 && isspace((unsigned char)s->data[i - 1])) i--;
    s->data[i] = '\0';
    s->len = i;
}

void eui_str_trim(eui_str_t *s) {
    eui_str_trim_right(s);
    eui_str_trim_left(s);
}

/* ---- substr ---- */

void eui_str_substr(eui_str_t *dst, const eui_str_t *src, size_t start, size_t len) {
    assert(start <= src->len);
    if (start + len > src->len) {
        len = src->len - start;
    }
    size_t need = len + 1;
    reserve(dst, need);
    if (src->data) {
        memcpy(dst->data, src->data + start, len);
    }
    dst->data[len] = '\0';
    dst->len = len;
}
