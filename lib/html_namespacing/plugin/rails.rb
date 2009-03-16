module HtmlNamespacing
  module Plugin
    module Rails
      def self.install(options = {})
        @options = options
        install_rails_2_3
      end

      # Called by ActionView
      def self.path_to_namespace(template)
        if func = @options[:template_to_namespace_callback]
          func.call(template)
        elsif template.path =~ /^([^\.]+)/
          $1.gsub('/', '-')
        end
      end

      private

      def self.install_rails_2_3(options = {})
        ::ActionView::Template.class_eval do
          def render_template_with_html_namespacing(*args)
            s = render_template_without_html_namespacing(*args)

            if format == 'html'
              HtmlNamespacing::add_namespace_to_html(s, html_namespace)
            else
              s
            end
          end

          alias_method_chain :render_template, :html_namespacing

          def html_namespace
            HtmlNamespacing::Plugin::Rails.path_to_namespace(self)
          end
          memoize :html_namespace
        end
      end
    end
  end
end
