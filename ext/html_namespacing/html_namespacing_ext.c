#include <errno.h>

#include "ruby.h"

#include "html_namespacing.h"

#include <stdio.h>

VALUE rb_mHtmlNamespacing = Qnil;

static const HtmlNamespacingAllocationStrategy ALLOCATION_STRATEGY = {
    (void *(*)(size_t)) ruby_xmalloc,
    ruby_xfree,
    (void *(*)(void *, size_t)) ruby_xrealloc
};

/*
 * call-seq:
 *   HtmlNamespacing.add_namespace_to_html(html, ns) => String
 *
 * Returns new HTML string based on +html+ with the  HTML class +ns+ to root
 * elements.
 *
 * If +html+ is nil, returns +nil+.
 *
 * If +ns+ is nil, returns +html+.
 */

VALUE
html_namespacing_add_namespace_to_html(
        VALUE obj,
        VALUE html,
        VALUE ns)
{
    /*
     * It's almost tempting to manually allocate the RString object to save
     * ourselves the stupid extra copy. (add_namespace_to_html_with_length()
     * implicitly copies the string, and here we are copying it again because
     * Ruby can't convert from a char* to an RString? How lame....)
     *
     * But for now, let's just do the extra copy (implicit in rb_str_new2) and
     * be done with it.
     */

    const char *html_ptr;
    size_t html_len;
    const char *ns_ptr;
    char *ret_ptr;
    size_t ret_len;
    int rv;
    VALUE ret;

    if (TYPE(html) == T_NIL) {
      return Qnil;
    }
    Check_Type(html, T_STRING);

    if (TYPE(ns) == T_NIL) {
      return html;
    }

    Check_Type(ns, T_STRING);

    html_ptr = RSTRING(html)->ptr;
    html_len = RSTRING(html)->len;

    ns_ptr = RSTRING(ns)->ptr;

    rv = add_namespace_to_html_with_length_and_allocation_strategy(
            html_ptr,
            html_len,
            ns_ptr,
            &ret_ptr,
            &ret_len,
            ALLOCATION_STRATEGY);
    if (rv == EINVAL) {
        rb_raise(rb_eArgError, "Badly-formed HTML");
    }
    if (rv != 0) {
        rb_raise(rb_eRuntimeError, "Unknown error in add_namespace_to_html");
    }

    ret = rb_str_new(ret_ptr, ret_len);
    ruby_xfree(ret_ptr);

    return ret;
}

/*
 * Holds functions related to HTML namespacing.
 */
void
Init_html_namespacing_ext()
{
    rb_mHtmlNamespacing = rb_define_module("HtmlNamespacing");
    rb_define_module_function(rb_mHtmlNamespacing, "add_namespace_to_html", html_namespacing_add_namespace_to_html, 2);
}
