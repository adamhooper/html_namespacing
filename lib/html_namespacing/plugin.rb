module HtmlNamespacing
  module Plugin
    autoload(:Rails, File.dirname(__FILE__) + '/plugin/rails')
    autoload(:Sass, File.dirname(__FILE__) + '/plugin/sass')

    def self.default_relative_path_to_namespace(path)
      if path =~ /^([^\.]+)/
        $1.gsub(/\//, '-')
      end
    end
  end
end
