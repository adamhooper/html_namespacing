#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

#include "html_namespacing.h"

#define WHITE_SPACE " \t\r\n"

static const char* const IGNORE_TAGS[] = {
    "html",
    "head",
    "base",
    "meta",
    "title",
    "link",
    "script",
    "noscript",
    "style",
    NULL
};

/*
 * Returns the first power of 2 which is >= the given length.
 */
static int
round_to_power_of_two(int i)
{
    int j = 1;

    while (j < i) {
        j <<= 1;
    }

    return j;
}

/*
 * Returns the number of bytes in the first utf-8 character of *utf8.
 */
static size_t
utf8_char_bytes(const char *utf8)
{
    size_t i;
    const unsigned char *u;

    if (utf8[0] < 0x80) {
        return 1;
    }

    u = (const unsigned char *) utf8;

    for (i = 1; i < 6; i++) {
        if ((u[i] & 0xc0) != 0x80) {
            return i;
        }
    }

    return -1;
}

/*
 * Copies num_bytes bytes from *src_p to *dest_p, doing no bounds checking.
 *
 * Advances *dest_p and *src_p by num_bytes.
 */
static void
copy_n_bytes_and_advance(char **dest_p, const char **src_p, size_t num_bytes)
{
    memcpy(*dest_p, *src_p, num_bytes);
    *dest_p += num_bytes;
    *src_p += num_bytes;
}

/*
 * Ensures *r is long enough to hold len chars (using realloc).
 *
 * Arguments:
 *   - char **r: Pointer to the string (which need not be \0-terminated). Will
 *     be replaced by the new string.
 *   - size_t *r_len: Pointer to the length of the string. Will be replaced by
 *     the new length, which will be >= len.
 *   - size_t len: Minimum length of the new string.
 *
 * Returns:
 *   - 0 on success, whether or not *r changed length.
 *   - ENOMEM if reallocation failed. *r and *r_len will point to their old
 *     values, which will remain unchanged.
 */
static int
ensure_string_length(
        char **r,
        size_t *r_len,
        size_t len,
        HtmlNamespacingAllocationStrategy allocation_strategy)
{
    char *new_r;
    size_t new_r_len;

    new_r_len = *r_len;
    if (new_r_len >= len) {
        /* No need to realloc */
        return 0;
    }

    while (new_r_len < len) {
        new_r_len <<= 1;
    }

    new_r = allocation_strategy.realloc(*r, sizeof(char) * new_r_len);
    if (!new_r) {
        return ENOMEM;
    }

    *r = new_r;
    *r_len = new_r_len;
    return 0;
}

/*
 * Tries to copy a single utf8 character to dest, possibly reallocating.
 *
 * Arguments:
 *   - dest: Beginning of destination string. May be reallocated during copy.
 *   - dest_len: Amount of memory allocated to dest. May be increased during
 *     copy.
 *   - dest_p: Pointer to end of destination string (potentially 1 past the
 *     allocated length of dest).
 *   - src_p: Source string. Will be advanced.
 *
 * Returns:
 *   - 0 on success. dest may be changed; dest_p will be incremented; src_p
 *     will be incremented.
 *   - ENOMEM if reallocation failed. dest, dest_p, and src_p will remain
 *     unchanged.
 */
static int
append_next_utf8_char(
        char **dest,
        size_t *dest_len,
        char **dest_p,
        const char **src_p,
        HtmlNamespacingAllocationStrategy allocation_strategy)
{
    size_t c_num_utf8_bytes;
    size_t dest_p_offset;
    size_t new_dest_len;
    char *new_dest;
    int rv;

    c_num_utf8_bytes = utf8_char_bytes(*src_p);
    new_dest = *dest;
    dest_p_offset = *dest_p - *dest;
    new_dest_len = dest_p_offset + c_num_utf8_bytes;

    rv = ensure_string_length(
            &new_dest, dest_len, new_dest_len, allocation_strategy);
    if (rv == ENOMEM) {
        return ENOMEM;
    }
    if (new_dest != *dest) {
        *dest = new_dest;
        *dest_p = new_dest + dest_p_offset;
    }

    copy_n_bytes_and_advance(dest_p, src_p, c_num_utf8_bytes);

    return 0;
}

/*
 * Tries to copy s into dest, possibly reallocating.
 *
 * Arguments:
 *   - dest: Beginning of destination string. May be reallocated during copy.
 *   - dest_len: Amount of memory allocated to dest. May be increased during
 *     copy.
 *   - dest_p: Pointer to end of destination string (potentially 1 past the
 *     allocated length of dest).
 *   - s: Source string.
 *
 * Returns:
 *   - 0 on success. dest may be changed; dest_p will be incremented.
 *   - ENOMEM if reallocation failed. dest and dest_p will remain unchanged.
 */
static int
append_string(
        char **dest,
        size_t *dest_len,
        char **dest_p,
        const char *s,
        HtmlNamespacingAllocationStrategy allocation_strategy)
{
    int len;
    int rv;
    size_t dest_p_offset;
    size_t new_dest_len;
    char *new_dest;

    len = strlen(s);
    new_dest = *dest;
    dest_p_offset = *dest_p - *dest;
    new_dest_len = dest_p_offset + len;

    rv = ensure_string_length(
            &new_dest, dest_len, new_dest_len, allocation_strategy);
    if (rv == ENOMEM) {
        return ENOMEM;
    }
    if (new_dest != *dest) {
        *dest = new_dest;
        *dest_p = new_dest + dest_p_offset;
    }

    strncpy(*dest_p, s, len);
    *dest_p += len;

    return 0;
};

static int
append_next_chars_until(
        char **dest,
        size_t *dest_len,
        char **dest_p,
        const char **src_p,
        const char *until_chars,
        HtmlNamespacingAllocationStrategy allocation_strategy)
{
    size_t num_bytes;
    size_t dest_p_offset;
    size_t new_dest_len;
    char *new_dest;
    int rv;

    num_bytes = strcspn(*src_p, until_chars);
    new_dest = *dest;
    dest_p_offset = *dest_p - *dest;
    new_dest_len = dest_p_offset + num_bytes;

    rv = ensure_string_length(
            &new_dest, dest_len, new_dest_len, allocation_strategy);
    if (rv == ENOMEM) {
        return ENOMEM;
    }
    if (new_dest != *dest) {
        *dest = new_dest;
        *dest_p = new_dest + dest_p_offset;
    }

    copy_n_bytes_and_advance(dest_p, src_p, num_bytes);

    return 0;
}

static int
append_end_of_string(
        char **dest,
        size_t *dest_len,
        char **dest_p,
        HtmlNamespacingAllocationStrategy allocation_strategy)
{
    int rv;
    const char *end = "\0";

    rv = append_next_utf8_char(
            dest, dest_len, dest_p, &end, allocation_strategy);

    if (rv == 0) {
        /* We don't add the length of '\0' */
        *dest_len -= 1;
        *dest_p -= 1;
    }

    return rv;
}

enum {
    PARSE_NORMAL,
    PARSE_OPEN_TAG,
    PARSE_OPEN_TAG_NAME,
    PARSE_OPEN_TAG_ATTRIBUTE_NAME,
    PARSE_OPEN_TAG_ATTRIBUTE_VALUE,
    PARSE_OPEN_TAG_ATTRIBUTE_EQUALS,
    PARSE_CLOSE_TAG,
    PARSE_EMPTY_TAG, /* detected while we're inside PARSE_OPEN_TAG */
    PARSE_COMMENT,
    PARSE_XML_DECL,
    PARSE_DOCTYPE,
    PARSE_CDATA
};

static int
char_is_whitespace(char c)
{
    return c == 0x20 || c == 0x09 || c == 0x0d || c == 0x0a;
}

static int
should_ignore_tag(const char *tag_name, size_t tag_len)
{
    int i = 0;
    const char *test_ignore;

    for (i = 0; (test_ignore = IGNORE_TAGS[i]); i++) {
        if (0 == strncmp(test_ignore, tag_name, tag_len)
                && strlen(test_ignore) == tag_len)
        {
            return 1;
        }
    }

    return 0;
}

/*
 * Adjusts the given HTML so that top-level elements get an added "class" of
 * namespace.
 *
 * For instance, given
 * "<div><span>Hello</span></div><p class="below">Goodbye</p>" and a
 * namespace of "foo", returns
 * "<div class="foo"><span>Hello</span></div><p class="below foo">Goodbye</p>".
 *
 * Arguments:
 *   - const char *html: Original HTML string. Must be valid HTML, encoded as
 *     utf-8.
 *   - int html_len: Length of HTML string.
 *   - const char *namespace: Namespace to add. Must be of the form
 *     "[a-zA-Z][-_a-zA-Z0-9]*".
 *   - char **ret: Pointer to return value, which will be newly-allocated. Will
 *     be overwritten only on success.
 *   - int *ret_len: Pointer to return value's length. Will be overwritten only
 *     on success.
 *
 * Returns:
 *   - 0 on success. *ret will be overwritten to point to the newly-allocated
 *     string.
 *   - ENOMEM if *ret could not be allocated.
 *   - EINVAL if html is not valid XML.
 */
int
add_namespace_to_html_with_length_and_allocation_strategy(
        const char *html,
        size_t html_len,
        const char *ns,
        char **ret,
        size_t *ret_len,
        HtmlNamespacingAllocationStrategy allocation_strategy)
{

#define APPEND_NEXT_CHAR() \
        if (*html_p && append_next_utf8_char(&r, &r_len, &r_p, &html_p, allocation_strategy) != 0) goto error/*;*/
#define APPEND_STRING(s) \
        if (append_string(&r, &r_len, &r_p, s, allocation_strategy) != 0) goto error/*;*/
#define APPEND_END_OF_STRING() \
        if (append_end_of_string(&r, &r_len, &r_p, allocation_strategy) != 0) goto error/*;*/
#define APPEND_NEXT_CHARS_UNTIL(chars) \
        if (append_next_chars_until(&r, &r_len, &r_p, &html_p, chars, allocation_strategy) != 0) goto error/*;*/

    unsigned int state;
    char *r; /* Start of retval */
    char *r_p; /* Pointer in retval */
    size_t r_len; /* Length of retval */
    const char *html_p;
    const char *open_tag_name = NULL;
    size_t open_tag_name_len = 0;
    size_t num_chars_remaining;
    int num_tags_open;
    int open_tag_attribute_is_class_attribute;
    int open_tag_had_class_attribute;
    const char *open_tag_attribute_value;

    r_len = round_to_power_of_two(html_len);
    r = allocation_strategy.malloc(sizeof(char) * r_len);
    if (!r) {
        return ENOMEM;
    }

    html_p = html;
    state = PARSE_NORMAL;
    r_p = r;
    num_tags_open = 0;
    open_tag_attribute_is_class_attribute = 0;
    open_tag_had_class_attribute = 0;
    open_tag_attribute_value = NULL;

    while (1) {
        num_chars_remaining = html_len - (html_p - html);
        if (num_chars_remaining <= 0) break;

        switch (state) {
            case PARSE_NORMAL:
                if (*html_p == '<') {
                    APPEND_NEXT_CHAR();
                    if (num_chars_remaining >= 9
                            && 0 == strncmp("![CDATA[", html_p, 8)) {
                        state = PARSE_CDATA;
                    } else if (num_chars_remaining >= 2 && html_p[0] == '/') {
                        state = PARSE_CLOSE_TAG;
                    } else if (num_chars_remaining >= 4
                            && 0 == strncmp("!--", html_p, 3)) {
                        state = PARSE_COMMENT;
                    } else if (num_chars_remaining >= 9
                            && 0 == strncmp("!DOCTYPE", html_p, 8)) {
                        state = PARSE_DOCTYPE;
                    } else if (num_chars_remaining >= 5
                            && 0 == strncmp("?xml", html_p, 4)) {
                        state = PARSE_XML_DECL;
                    } else {
                        open_tag_name = html_p;
                        state = PARSE_OPEN_TAG_NAME;
                    }
                } else {
                    APPEND_NEXT_CHARS_UNTIL("<");
                }
                break;
            case PARSE_OPEN_TAG_NAME:
                APPEND_NEXT_CHARS_UNTIL(WHITE_SPACE ">/");
                open_tag_name_len = html_p - open_tag_name;
                state = PARSE_OPEN_TAG;
                break;
            case PARSE_OPEN_TAG:
                if (*html_p == '/' || *html_p == '>') {

                    if (num_tags_open == 0 && !open_tag_had_class_attribute
                            && !should_ignore_tag(open_tag_name, open_tag_name_len)) {
                        if (*html_p == '/' && char_is_whitespace(*(html_p - 1))) {
                            /* We're in an empty tag with a trailing space */
                            APPEND_STRING("class=\"");
                            APPEND_STRING(ns);
                            APPEND_STRING("\" ");
                        } else {
                            APPEND_STRING(" class=\"");
                            APPEND_STRING(ns);
                            APPEND_STRING("\"");
                        }
                    }

                    open_tag_had_class_attribute = 0;
                    open_tag_attribute_value = NULL;

                    if (*html_p == '/') {
                        state = PARSE_EMPTY_TAG;
                    } else {
                        num_tags_open++;
                        state = PARSE_NORMAL;
                    }
                    APPEND_NEXT_CHAR();
                } else if (!char_is_whitespace(*html_p)) {
                    if (num_chars_remaining >= 5
                            && 0 == strncmp(html_p, "class", 5)) {
                        open_tag_attribute_is_class_attribute = 1;
                        open_tag_had_class_attribute = 1;
                    } else {
                        open_tag_attribute_is_class_attribute = 0;
                    }
                    state = PARSE_OPEN_TAG_ATTRIBUTE_NAME;
                } else {
                    APPEND_NEXT_CHAR();
                }
                break;
            case PARSE_OPEN_TAG_ATTRIBUTE_NAME:
                APPEND_NEXT_CHARS_UNTIL("=");
                state = PARSE_OPEN_TAG_ATTRIBUTE_EQUALS;
                APPEND_NEXT_CHAR();
                break;
            case PARSE_OPEN_TAG_ATTRIBUTE_EQUALS:
                APPEND_NEXT_CHARS_UNTIL("'\"");
                open_tag_attribute_value = html_p;
                state = PARSE_OPEN_TAG_ATTRIBUTE_VALUE;
                APPEND_NEXT_CHAR();
                break;
            case PARSE_OPEN_TAG_ATTRIBUTE_VALUE:
                APPEND_NEXT_CHARS_UNTIL("'\"");
                /* open_tag_attribute_value is either ' or " */
                while (*html_p != *open_tag_attribute_value) {
                    APPEND_NEXT_CHAR();
                    APPEND_NEXT_CHARS_UNTIL("'\"");
                }
                if (open_tag_attribute_is_class_attribute
                        && num_tags_open == 0) {
                    APPEND_STRING(" ");
                    APPEND_STRING(ns);
                }
                open_tag_attribute_is_class_attribute = 0;
                state = PARSE_OPEN_TAG;
                APPEND_NEXT_CHAR();
                break;
            case PARSE_CLOSE_TAG:
                APPEND_NEXT_CHARS_UNTIL(">");
                num_tags_open--;
                open_tag_attribute_value = NULL;
                state = PARSE_NORMAL;
                APPEND_NEXT_CHAR();
                break;
            case PARSE_EMPTY_TAG:
            case PARSE_XML_DECL:
            case PARSE_DOCTYPE:
                APPEND_NEXT_CHARS_UNTIL(">");
                state = PARSE_NORMAL;
                APPEND_NEXT_CHAR();
                break;
            case PARSE_COMMENT:
                APPEND_NEXT_CHAR(); /* at least one */
                APPEND_NEXT_CHARS_UNTIL("-");
                if (*html_p == '-' && num_chars_remaining >= 3
                        && 0 == strncmp("->", html_p, 2)) {
                    APPEND_NEXT_CHAR();
                    APPEND_NEXT_CHAR();
                    state = PARSE_NORMAL;
                }
                /* else loop... */
                break;
            case PARSE_CDATA:
                APPEND_NEXT_CHAR(); /* at least one */
                APPEND_NEXT_CHARS_UNTIL("]");
                if (*html_p == ']' && num_chars_remaining >= 3
                        && 0 == strncmp("]>", html_p, 2)) {
                    APPEND_NEXT_CHAR();
                    APPEND_NEXT_CHAR();
                    state = PARSE_NORMAL;
                }
                /* else loop... */
                break;
            default:
                assert(0);
        }
    }

    APPEND_END_OF_STRING();

    if (state != PARSE_NORMAL
            || (r_p - r) < html_len
            || num_chars_remaining != 0
            || num_tags_open != 0
            || open_tag_attribute_is_class_attribute
            || open_tag_attribute_value)
    {
        allocation_strategy.free (r);
        *ret = NULL;
        *ret_len = -1;
        return EINVAL;
    }

    *ret = r;
    *ret_len = r_p - r;
    return 0;

error:
    allocation_strategy.free(r);
    *ret = NULL;
    *ret_len = -1;
    return ENOMEM;
}

int
add_namespace_to_html_with_length(
        const char *html,
        size_t html_len,
        const char *ns,
        char **ret,
        size_t *ret_len)
{
    HtmlNamespacingAllocationStrategy allocation_strategy;
    allocation_strategy.malloc = malloc;
    allocation_strategy.free = free;
    allocation_strategy.realloc = realloc;

    return add_namespace_to_html_with_length_and_allocation_strategy(
            html, html_len, ns, ret, ret_len, allocation_strategy);
}


#if INCLUDE_MAIN
#include <stdio.h>

int
main(int argc, char **argv)
{
    const char *html;
    size_t html_len;
    const char *ns;
    char *ret;
    size_t ret_len;
    int rv;
    HtmlNamespacingAllocationStrategy allocation_strategy;

    if (argc != 3) {
        printf("usage: %s HTML_STRING NAMESPACE", argv[0]);
        return 1;
    }
   
    html = argv[1];
    html_len = strlen(html);
    ns = argv[2];

    printf("html: %s\n", html);
    printf("html_len: %d\n", html_len);
    printf("ns: %s\n", ns);

    allocation_strategy.malloc = malloc;
    allocation_strategy.free = free;
    allocation_strategy.realloc = realloc;

    rv = add_namespace_to_html_with_length_and_allocation_strategy(html, html_len, ns, &ret, &ret_len, allocation_strategy);

    if (ret) {
        printf("ret: %s\n", ret);
        printf("ret_len: %d\n", ret_len);
    } else {
        printf("Failed!\n");
    }

    return rv;
}
#endif /* INCLUDE_MAIN */
