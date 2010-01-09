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

static int
determine_alloc_size_of_at_least(int i)
{
    return i + 1024;
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
        size_t len,
        HtmlNamespacingAllocationStrategy allocation_strategy)
{
    int rv;
    size_t dest_p_offset;
    size_t new_dest_len;
    char *new_dest;

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

#define APPEND_STRING(s) \
        do { \
            if (append_string(&r, &r_len, &r_p, s, strlen(s), allocation_strategy) != 0) goto error; \
        } while (0)/*;*/
#define APPEND_END_OF_STRING() \
        do { \
            if (append_string(&r, &r_len, &r_p, "", 1, allocation_strategy) != 0) goto error; \
            r_len -= 1; \
            r_p -= 1; \
        } while (0)/*;*/
#define COPY_TO_HERE() \
        do { \
            if (append_string(&r, &r_len, &r_p, html_first_uncopied_p, html_p - html_first_uncopied_p, allocation_strategy) != 0) goto error; \
            html_first_uncopied_p = html_p; \
        } while (0)/*;*/
#define ADVANCE(n) \
        do { \
            if (html_p + n <= html + html_len) { \
                html_p += n; \
            } else { \
                html_p = html + html_len; \
            } \
        } while (0)/*;*/
#define ADVANCE_UNTIL_ONE_OF_THESE_CHARS(chars) \
        ADVANCE(strcspn(html_p, chars))/*;*/

    unsigned int state;
    char *r; /* Start of retval */
    char *r_p; /* Pointer in retval */
    size_t r_len; /* Length of retval */
    const char *html_first_uncopied_p;
    const char *html_p;
    const char *open_tag_name = NULL;
    size_t open_tag_name_len = 0;
    size_t num_chars_remaining;
    int num_tags_open;
    int open_tag_attribute_is_class_attribute;
    int open_tag_had_class_attribute;
    const char *open_tag_attribute_value;

    r_len = determine_alloc_size_of_at_least(html_len);
    r = allocation_strategy.malloc(sizeof(char) * r_len);
    if (!r) {
        return ENOMEM;
    }

    html_first_uncopied_p = html_p = html;
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
                    ADVANCE(1);
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
                    ADVANCE_UNTIL_ONE_OF_THESE_CHARS("<");
                }
                break;
            case PARSE_OPEN_TAG_NAME:
                ADVANCE_UNTIL_ONE_OF_THESE_CHARS(WHITE_SPACE ">/");
                open_tag_name_len = html_p - open_tag_name;
                state = PARSE_OPEN_TAG;
                break;
            case PARSE_OPEN_TAG:
                if (*html_p == '/' || *html_p == '>') {
                    if (num_tags_open == 0 && !open_tag_had_class_attribute
                            && !should_ignore_tag(open_tag_name, open_tag_name_len)) {
                        COPY_TO_HERE();
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
                    ADVANCE(1);
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
                    ADVANCE(1);
                }
                break;
            case PARSE_OPEN_TAG_ATTRIBUTE_NAME:
                ADVANCE_UNTIL_ONE_OF_THESE_CHARS("=");
                ADVANCE(1);
                state = PARSE_OPEN_TAG_ATTRIBUTE_EQUALS;
                break;
            case PARSE_OPEN_TAG_ATTRIBUTE_EQUALS:
                ADVANCE_UNTIL_ONE_OF_THESE_CHARS("'\"");
                open_tag_attribute_value = html_p;
                state = PARSE_OPEN_TAG_ATTRIBUTE_VALUE;
                ADVANCE(1);
                break;
            case PARSE_OPEN_TAG_ATTRIBUTE_VALUE:
                ADVANCE_UNTIL_ONE_OF_THESE_CHARS("'\"");
                /* open_tag_attribute_value is either ' or " */
                while (*html_p != *open_tag_attribute_value) {
                    ADVANCE(1);
                    ADVANCE_UNTIL_ONE_OF_THESE_CHARS("'\"");
                }
                if (open_tag_attribute_is_class_attribute
                        && num_tags_open == 0) {
                    COPY_TO_HERE();
                    APPEND_STRING(" ");
                    APPEND_STRING(ns);
                }
                open_tag_attribute_is_class_attribute = 0;
                state = PARSE_OPEN_TAG;
                ADVANCE(1);
                break;
            case PARSE_CLOSE_TAG:
                ADVANCE_UNTIL_ONE_OF_THESE_CHARS(">");
                num_tags_open--;
                open_tag_attribute_value = NULL;
                state = PARSE_NORMAL;
                ADVANCE(1);
                break;
            case PARSE_EMPTY_TAG:
            case PARSE_XML_DECL:
            case PARSE_DOCTYPE:
                ADVANCE_UNTIL_ONE_OF_THESE_CHARS(">");
                ADVANCE(1);
                state = PARSE_NORMAL;
                break;
            case PARSE_COMMENT:
                ADVANCE(1); /* at least one */
                ADVANCE_UNTIL_ONE_OF_THESE_CHARS("-");
                if (*html_p == '-' && num_chars_remaining >= 3
                        && 0 == strncmp("->", ++html_p, 2)) {
                    ADVANCE(2);
                    state = PARSE_NORMAL;
                }
                /* else loop... */
                break;
            case PARSE_CDATA:
                ADVANCE(1); /* at least one */
                ADVANCE_UNTIL_ONE_OF_THESE_CHARS("]");
                if (*html_p == ']' && num_chars_remaining >= 3
                        && 0 == strncmp("]>", ++html_p, 2)) {
                    ADVANCE(2);
                    state = PARSE_NORMAL;
                }
                /* else loop... */
                break;
            default:
                assert(0);
        }
    }

    COPY_TO_HERE();
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
