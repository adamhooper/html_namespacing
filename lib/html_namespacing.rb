require File.dirname(__FILE__) + '/../ext/html_namespacing/html_namespacing_ext'

module HtmlNamespacing
  autoload(:Plugin, File.dirname(__FILE__) + '/html_namespacing/plugin')

  def self.options
    @options ||= {}
  end
end
