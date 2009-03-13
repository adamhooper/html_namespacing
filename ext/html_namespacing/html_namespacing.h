#ifndef HTML_NAMESPACING_H
#define HTML_NAMESPACING_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _HtmlNamespacingAllocationStrategy {
    void *(*malloc)(size_t size);
    void (*free)(void *ptr);
    void *(*realloc)(void *ptr, size_t size);
} HtmlNamespacingAllocationStrategy;

int
add_namespace_to_html_with_length(
        const char *html,
        size_t html_len,
        const char *ns,
        char **ret,
        size_t *ret_len);

int
add_namespace_to_html_with_length_and_allocation_strategy(
        const char *html,
        size_t html_len,
        const char *ns,
        char **ret,
        size_t *ret_len,
        HtmlNamespacingAllocationStrategy allocation_strategy);

#ifdef __cplusplus
}
#endif

#endif /* HTML_NAMESPACING_H */
