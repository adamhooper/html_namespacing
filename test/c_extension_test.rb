#!/usr/bin/env ruby

require File.dirname(__FILE__) + '/test_helper'

class CExtensionTest < Test::Unit::TestCase
  private

  def self.define_test(name, html, ns, expect)
    define_method("test_#{name}".to_sym) do
      assert_equal(expect, f(html, ns))
    end
  end

  def self.define_failing_test(name, html)
    define_method("test_#{name}".to_sym) do
      assert_raises(ArgumentError) do
        f(html, 'X')
      end
    end
  end

  self.define_test('nil HTML', nil, 'X', nil)
  self.define_test('nil namespace', '<div>hello</div>', nil, '<div>hello</div>')
  self.define_test('nil everything', nil, nil, nil)
  self.define_test('plain text', 'hello', 'X', 'hello')
  self.define_test('regular tag', '<div>hello</div>', 'X', '<div class="X">hello</div>')
  self.define_test('empty tag', '<div/>', 'X', '<div class="X"/>')
  self.define_test('empty tag with " />"', '<div />', 'X', '<div class="X" />')
  self.define_test('empty tag with " />" and class', '<div class="A" />', 'X', '<div class="A X" />')
  self.define_test('nested tag', '<div><div>hello</div></div>', 'X', '<div class="X"><div>hello</div></div>')
  self.define_test('two tags', '<div>hello</div><div>goodbye</div>', 'X', '<div class="X">hello</div><div class="X">goodbye</div>')
  self.define_test('existing class= tag with double-quotes', '<div class="foo">bar</div>', 'baz', '<div class="foo baz">bar</div>')
  self.define_test('existing class= tag with single-quotes', "<div class='foo'>bar</div>", 'baz', "<div class='foo baz'>bar</div>")
  self.define_test('other attributes are ignored', '<div id="id" class="foo" style="display:none;">bar</div>', 'baz', '<div id="id" class="foo baz" style="display:none;">bar</div>')
  self.define_test('works with utf-8', '<div class="𝞪">𝟂</div>', '𝞺', '<div class="𝞪 𝞺">𝟂</div>')
  self.define_test('empty tag with existing class=', '<span class="foo"/>', 'bar', '<span class="foo bar"/>')
  self.define_test('works with newlines in tag', "<div\n\nclass\n\n=\n\n'foo'\n\n>bar</div>", 'baz', "<div\n\nclass\n\n=\n\n'foo baz'\n\n>bar</div>")
  self.define_test('works with "\'" within \'"\' attributes', '<div title="Adam\'s House" class="foo">bar</div>', 'baz', '<div title="Adam\'s House" class="foo baz">bar</div>')
  self.define_test('ignores XML prolog', '<?xml version="1.0"?><div>foo</div>', 'X', '<?xml version="1.0"?><div class="X">foo</div>')
  self.define_test('ignores DOCTYPE', '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"><div>foo</div>', 'X', '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"><div class="X">foo</div>')
  self.define_test('ignores CDATA', '<![CDATA[ignore <div class="foo">]] </div>]]><div>foo</div>', 'X', '<![CDATA[ignore <div class="foo">]] </div>]]><div class="X">foo</div>')
  self.define_test('ignores comments', '<!-- blah <div class="foo">foo</div> - <span/>--><div>foo</div>', 'X', '<!-- blah <div class="foo">foo</div> - <span/>--><div class="X">foo</div>')

  [ 'html', 'head', 'base', 'meta', 'title', 'link', 'script', 'noscript', 'style' ].each do |tag|
    self.define_test("ignores <#{tag}> tags", "<#{tag}>foo</#{tag}>", "X", "<#{tag}>foo</#{tag}>")
  end

  self.define_failing_test('unclosed tag fails', '<div>foo')
  self.define_failing_test('closing tag fails', 'foo</div>')
  self.define_failing_test('wrong attr syntax fails', '<div foo=bar>foo</div>')

  def f(html, ns)
    HtmlNamespacing::add_namespace_to_html(html, ns)
  end
end
