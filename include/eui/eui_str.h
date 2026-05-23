#ifndef EUI_STR_H
#define EUI_STR_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char  *data;
    size_t len;
    size_t cap;
} eui_str_t;

void        eui_str_init(eui_str_t *s);
void        eui_str_clear(eui_str_t *s);
eui_str_t*  eui_str_create(void);
void        eui_str_destroy(eui_str_t *s);

void        eui_str_set(eui_str_t *s, const char *src);
void        eui_str_copy(eui_str_t *dst, const eui_str_t *src);

void        eui_str_append(eui_str_t *s, const char *src);
void        eui_str_append_str(eui_str_t *s, const eui_str_t *src);
void        eui_str_append_char(eui_str_t *s, char c);

void        eui_str_printf(eui_str_t *s, const char *fmt, ...);
void        eui_str_append_printf(eui_str_t *s, const char *fmt, ...);

const char* eui_str_cstr(const eui_str_t *s);
size_t      eui_str_len(const eui_str_t *s);
bool        eui_str_empty(const eui_str_t *s);

bool        eui_str_equals(const eui_str_t *a, const char *b);
bool        eui_str_equals_str(const eui_str_t *a, const eui_str_t *b);
int         eui_str_cmp(const eui_str_t *a, const eui_str_t *b);

void        eui_str_trim_left(eui_str_t *s);
void        eui_str_trim_right(eui_str_t *s);
void        eui_str_trim(eui_str_t *s);
void        eui_str_substr(eui_str_t *dst, const eui_str_t *src, size_t start, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* EUI_STR_H */
