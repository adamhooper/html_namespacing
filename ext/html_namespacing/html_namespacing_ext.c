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

static VALUE
html_namespacing_add_namespace_to_html(
        VALUE obj,
        VALUE html_value,
        VALUE ns_value)
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
    const char *html;
    size_t html_len;
    const char *ns;
    char *ret;
    size_t ret_len;
    int rv;
    VALUE ret_value;

    if (TYPE(html_value) == T_NIL) {
      return Qnil;
    }
    Check_Type(html_value, T_STRING);

    if (TYPE(ns_value) == T_NIL) {
      return html_value;
    }

    Check_Type(ns_value, T_STRING);

    html = RSTRING(html_value)->ptr;
    html_len = RSTRING(html_value)->len;

    ns = RSTRING(ns_value)->ptr;

    rv = add_namespace_to_html_with_length_and_allocation_strategy(
            html, html_len, ns, &ret, &ret_len, ALLOCATION_STRATEGY);
    if (rv == EINVAL) {
        rb_raise(rb_eArgError, "Badly-formed HTML");
    }
    if (rv != 0) {
        rb_raise(rb_eRuntimeError, "Unknown error in add_namespace_to_html");
    }

    ret_value = rb_str_new(ret, ret_len);
    ruby_xfree(ret);

    return ret_value;
}

void
Init_html_namespacing_ext()
{
    rb_mHtmlNamespacing = rb_define_module("HtmlNamespacing");
    rb_define_module_function(rb_mHtmlNamespacing, "add_namespace_to_html", html_namespacing_add_namespace_to_html, 2);
}
