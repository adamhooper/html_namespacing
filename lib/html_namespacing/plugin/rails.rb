module HtmlNamespacing
  module Plugin
    module Rails
      def self.install(options = {})
        @options = options
        install_rails_2_3(options)
      end

      # Called by ActionView
      def self.path_to_namespace(relative_path)
        if func = @options[:path_to_namespace_callback]
          func.call(relative_path)
        else
          HtmlNamespacing::Plugin.default_relative_path_to_namespace(relative_path)
        end
      end

      def self.handle_exception(e, template, view)
        if func = @options[:handle_exception_callback]
          func.call(e, template, view)
        else
          raise(e)
        end
      end

      def self.javascript_root
        @options[:javascript_root] || RAILS_ROOT + '/app/javascripts/views'
      end

      def self.javascript_optional_suffix
        @options[:javascript_optional_suffix]
      end

      def self.template_formats
        @formats ||= Set.new(@options[:template_formats] || ['html'])
      end

      module Helpers
        def html_namespacing_javascript_tag(framework, options = {})
          files = html_namespacing_files(options[:format])
          root = Pathname.new(::HtmlNamespacing::Plugin::Rails.javascript_root)

          unless files.empty?
            r = "<script type=\"text/javascript\"><!--//--><![CDATA[//><!--\n"
            r << HtmlNamespaceJs[framework][:top]
            r << "\n"
            files.collect do |path|
              relative_path = Pathname.new(path).relative_path_from(root)
              r << html_namespacing_inline_js(framework, relative_path.to_s, path)
              r << "\n"
            end
            r << "//--><!]]></script>"
          end
        end

        private

        HtmlNamespaceJs = {
          :jquery => {
            :top => File.open(File.join(File.dirname(__FILE__), 'dom_scan_jquery.js.compressed')) { |f| f.read },
            :each => 'jQuery(function($){var NS=%s,$NS=function(){return $($.NS[NS]||[])};%s});'
          }
        }

        def html_namespacing_files(format)
          (Array === format ? format : [format]).inject([]) do |r, f|
            r.concat(GlobFu.find(
              html_namespacing_rendered_paths,
              :suffix => 'js',
              :extra_suffix => f && f.to_s,
              :optional_suffix => ::HtmlNamespacing::Plugin::Rails.javascript_optional_suffix,
              :root => ::HtmlNamespacing::Plugin::Rails.javascript_root
            ))
          end
        end

        def html_namespacing_inline_js(framework, relative_path, absolute_path)
          namespace = ::HtmlNamespacing::Plugin::Rails.path_to_namespace(relative_path.sub(/\..*$/, ''))
          content = File.open(absolute_path) { |f| f.read }

          HtmlNamespaceJs[framework][:each] % [namespace.to_json, content]
        end
      end

      private

      def self.install_rails_2_3(options = {})
        if options[:javascript]
          ::ActionView::Base.class_eval do
            attr_writer(:html_namespacing_rendered_paths)
            def html_namespacing_rendered_paths
              @html_namespacing_rendered_paths ||= []
            end
          end
        end

        ::ActionView::Template.class_eval do
          def render_with_html_namespacing(view, local_assigns = {})
            html = render_without_html_namespacing(view, local_assigns)

            view.html_namespacing_rendered_paths << path_without_format_and_extension if view.respond_to?(:html_namespacing_rendered_paths)

            if HtmlNamespacing::Plugin::Rails.template_formats.include?(format)
              add_namespace_to_html(html, view)
            else
              html
            end
          end
          alias_method_chain :render, :html_namespacing

          private

          def add_namespace_to_html(html, view)
            HtmlNamespacing::add_namespace_to_html(html, html_namespace)
          rescue ArgumentError => e
            HtmlNamespacing::Plugin::Rails.handle_exception(e, self, view)
            html # unless handle_exception() raised something
          end

          def html_namespace
            HtmlNamespacing::Plugin::Rails.path_to_namespace(path_without_format_and_extension)
          end
        end
      end
    end
  end
end
