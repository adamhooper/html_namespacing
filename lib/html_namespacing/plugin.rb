module HtmlNamespacing
  module Plugin
    autoload(:Rails, File.dirname(__FILE__) + '/plugin/rails')
    autoload(:Sass, File.dirname(__FILE__) + '/plugin/sass')

    def self.default_relative_path_to_namespace(path)
      path.gsub(/\//, '-')
    end
  end
end
