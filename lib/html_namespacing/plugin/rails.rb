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
        else
          HtmlNamespacing::Plugin.default_relative_path_to_namespace(template.path)
        end
      end

      def self.handle_exception(e, template, view)
        if func = @options[:handle_exception_callback]
          func.call(e, template, view)
        else
          raise(e)
        end
      end

      private

      def self.install_rails_2_3(options = {})
        ::ActionView::Template.class_eval do
          def render_template_with_html_namespacing(*args)
            s = render_template_without_html_namespacing(*args)
            namespace_rendered_content(s)
          end
          alias_method_chain :render_template, :html_namespacing

          def render_with_html_namespacing(view, local_assigns={})
            s = render_without_html_namespacing(view, local_assigns)
            namespace_rendered_content(s)
          end
          alias_method_chain :render, :html_namespacing

          def namespace_rendered_content(content)
            if format == 'html'
              begin
                HtmlNamespacing::add_namespace_to_html(content, html_namespace)
              rescue ArgumentError => e
                view = args.first
                HtmlNamespacing::Plugin::Rails.handle_exception(e, self, view)
                content # unless handle_exception() raised something
              end
            else
              content
            end
          end

          def html_namespace
            HtmlNamespacing::Plugin::Rails.path_to_namespace(self)
          end
          memoize :html_namespace
        end
      end
    end
  end
end
