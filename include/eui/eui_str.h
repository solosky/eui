#ifndef EUI_STR_H
#define EUI_STR_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Dynamic string type.
 *
 * A growable, null-terminated string backed by a heap-allocated buffer.
 * Instances can be stack-allocated (use eui_str_init()) or heap-allocated
 * (use eui_str_create()).
 */
typedef struct {
    char  *data;  /**< Null-terminated character buffer (may be NULL when empty). */
    size_t len;   /**< Length of the string (not including the null terminator). */
    size_t cap;   /**< Allocated capacity of the buffer (not including the null terminator). */
} eui_str_t;

/**
 * @brief Initialize a stack-allocated eui_str_t as an empty string.
 *
 * @param s  Pointer to an uninitialized eui_str_t on the stack.
 *
 * @note Does not allocate heap memory; the string starts empty and
 *       will allocate on first write.
 */
void        eui_str_init(eui_str_t *s);

/**
 * @brief Clear the string content and free the internal buffer.
 *
 * After this call the string is empty and @p s->data is NULL.
 *
 * @param s  Pointer to an initialized eui_str_t.
 */
void        eui_str_clear(eui_str_t *s);

/**
 * @brief Allocate and create a new eui_str_t on the heap.
 *
 * @return A pointer to a newly heap-allocated eui_str_t, or NULL on
 *         allocation failure.  The string starts empty.
 *
 * @note The returned string must be freed with eui_str_destroy().
 *
 * @see eui_str_destroy()
 */
eui_str_t*  eui_str_create(void);

/**
 * @brief Destroy a heap-allocated string previously created by eui_str_create().
 *
 * Frees both the internal buffer and the eui_str_t structure itself.
 *
 * @param s  Pointer to a string created by eui_str_create().
 *
 * @see eui_str_create()
 */
void        eui_str_destroy(eui_str_t *s);

/**
 * @brief Set the string content from a null-terminated C string.
 *
 * Replaces any existing content.
 *
 * @param s    Target string.
 * @param src  Source C string (may be NULL, in which case the string
 *             becomes empty).
 */
void        eui_str_set(eui_str_t *s, const char *src);

/**
 * @brief Copy content from another eui_str_t.
 *
 * @param dst  Destination string.
 * @param src  Source string to copy from.
 */
void        eui_str_copy(eui_str_t *dst, const eui_str_t *src);

/**
 * @brief Append a null-terminated C string to the end.
 *
 * @param s    Target string.
 * @param src  C string to append.
 */
void        eui_str_append(eui_str_t *s, const char *src);

/**
 * @brief Append another eui_str_t to the end.
 *
 * @param s    Target string.
 * @param src  Source eui_str_t to append.
 */
void        eui_str_append_str(eui_str_t *s, const eui_str_t *src);

/**
 * @brief Append a single character to the end.
 *
 * @param s  Target string.
 * @param c  Character to append.
 */
void        eui_str_append_char(eui_str_t *s, char c);

/**
 * @brief Replace the string content using a printf-style format string.
 *
 * @param s    Target string.
 * @param fmt  Printf-style format string.
 * @param ...  Arguments for the format string.
 */
void        eui_str_printf(eui_str_t *s, const char *fmt, ...);

/**
 * @brief Append formatted text using a printf-style format string.
 *
 * @param s    Target string.
 * @param fmt  Printf-style format string.
 * @param ...  Arguments for the format string.
 */
void        eui_str_append_printf(eui_str_t *s, const char *fmt, ...);

/**
 * @brief Get the underlying null-terminated C string.
 *
 * The returned pointer is valid until the next modifying operation on @p s.
 *
 * @param s  Source string.
 * @return A null-terminated C string (never NULL; empty string if no content).
 */
const char* eui_str_cstr(const eui_str_t *s);

/**
 * @brief Get the current string length.
 *
 * @param s  Source string.
 * @return The number of characters in the string (not including the null
 *         terminator).
 */
size_t      eui_str_len(const eui_str_t *s);

/**
 * @brief Check if the string is empty (zero length).
 *
 * @param s  Source string.
 * @return true if the string has no characters, false otherwise.
 */
bool        eui_str_empty(const eui_str_t *s);

/**
 * @brief Compare the string with a C string for equality.
 *
 * @param a  eui_str_t to compare.
 * @param b  C string to compare against.
 * @return true if both strings have identical content.
 */
bool        eui_str_equals(const eui_str_t *a, const char *b);

/**
 * @brief Compare two eui_str_t for equality.
 *
 * @param a  First string.
 * @param b  Second string.
 * @return true if both strings have identical content.
 */
bool        eui_str_equals_str(const eui_str_t *a, const eui_str_t *b);

/**
 * @brief Compare two eui_str_t (strcmp-style).
 *
 * @param a  First string.
 * @param b  Second string.
 * @return A negative value if @p a < @p b, zero if equal, positive otherwise.
 */
int         eui_str_cmp(const eui_str_t *a, const eui_str_t *b);

/**
 * @brief Remove leading whitespace characters (space, tab, newline, etc.).
 *
 * @param s  String to trim in place.
 */
void        eui_str_trim_left(eui_str_t *s);

/**
 * @brief Remove trailing whitespace characters.
 *
 * @param s  String to trim in place.
 */
void        eui_str_trim_right(eui_str_t *s);

/**
 * @brief Remove both leading and trailing whitespace characters.
 *
 * @param s  String to trim in place.
 */
void        eui_str_trim(eui_str_t *s);

/**
 * @brief Extract a substring and store it in @p dst.
 *
 * Copies up to @p len characters from @p src starting at offset @p start.
 * If @p start exceeds the source length, @p dst becomes empty.
 *
 * @param dst    Destination string (must be initialized).
 * @param src    Source string.
 * @param start  Zero-based start index.
 * @param len    Maximum number of characters to copy.
 */
void        eui_str_substr(eui_str_t *dst, const eui_str_t *src, size_t start, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* EUI_STR_H */
