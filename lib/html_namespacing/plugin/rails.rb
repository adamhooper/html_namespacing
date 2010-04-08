begin
  require 'glob_fu'
rescue LoadError
  require 'rubygems'
  require 'glob_fu'
end

module HtmlNamespacing
  module Plugin
    module Rails
      def self.install(options = {})
        @options = options
        if ::Rails::VERSION::MAJOR == 3
          install_rails_3(options)
        else
          install_rails_2_3(options)
        end
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

      def self.stylesheet_root
        @options[:stylesheet_root] || RAILS_ROOT + '/app/stylesheets/views'
      end

      def self.javascript_optional_suffix
        @options[:javascript_optional_suffix]
      end

      def self.stylesheet_optional_suffix
        @options[:stylesheet_optional_suffix]
      end

      def self.template_formats
        @formats ||= Set.new((@options[:template_formats] || [:html]).collect(&:to_sym))
      end

      module Helpers
        #
        # Return the javascript for the rendered partials, wrapped in
        # a script tag.
        #
        def html_namespacing_javascript_tag(framework, options = {})
          js = html_namespacing_javascript(framework, options)
          unless js.blank?
            s = "<script type=\"text/javascript\"><!--//--><![CDATA[//><!--\n#{js}//--><!]]></script>"
            s.respond_to?(:html_safe) && s.html_safe || s
          end
        end

        #
        # Return the javascript for the rendered partials.
        #
        def html_namespacing_javascript(framework, options = {})
          root = Pathname.new(Rails.javascript_root)
          optional_suffix = Rails.javascript_optional_suffix
          files = html_namespacing_files('js',
                                         :format => options[:format],
                                         :optional_suffix => optional_suffix,
                                         :root => root.to_s)
          unless files.empty?
            r = [HtmlNamespaceJs[framework][:top]] << "\n"
            files.each do |path|
              relative_path = Pathname.new(path).relative_path_from(root)
              r << html_namespacing_inline_js(framework, relative_path.to_s, path) << "\n"
            end
            r.join
          end
        end

        #
        # Return the CSS for the rendered partials, wrapped in a style
        # tag.
        #
        def html_namespacing_style_tag(options = {}, style_tag_attributes = {})
          css = html_namespacing_styles(options)
          unless css.blank?
            attribute_string = style_tag_attributes.map{|k,v| " #{k}=\"#{v}\""}.join
            s = "<style type=\"text/css\"#{attribute_string}>#{css}</style>"
            s.respond_to?(:html_safe) && s.html_safe || s
          end
        end

        #
        # Return the CSS for the rendered partials.
        #
        def html_namespacing_styles(options = {})
          root = Pathname.new(Rails.stylesheet_root)
          optional_suffix = Rails.stylesheet_optional_suffix
          files = html_namespacing_files('sass',
                                         :format => options[:format],
                                         :optional_suffix => optional_suffix,
                                         :root => root.to_s)
          unless files.empty?
            r = []
            files.each do |path|
              relative_path = Pathname.new(path).relative_path_from(root)
              r << html_namespacing_inline_css(relative_path.to_s, path)
            end
            r.join
          end
        end

        private

        HtmlNamespaceJs = {
          :jquery => {
            :top => File.open(File.join(File.dirname(__FILE__), 'dom_scan_jquery.js.compressed')) { |f| f.read },
            :each => 'jQuery(function($){var NS=%s,$NS=function(){return $($.NS[NS]||[])};%s});'
          }
        }

        def html_namespacing_files(suffix, options={})
          format = options[:format]
          (Array === format ? format : [format]).inject([]) do |r, f|
            r.concat(GlobFu.find(
              html_namespacing_rendered_paths,
              :suffix => suffix,
              :extra_suffix => f && f.to_s,
              :optional_suffix => options[:optional_suffix],
              :root => options[:root]
            ))
          end
        end

        def html_namespacing_inline_js(framework, relative_path, absolute_path)
          namespace = ::HtmlNamespacing::Plugin::Rails.path_to_namespace(relative_path.sub(/\..*$/, ''))
          content = File.open(absolute_path) { |f| f.read }

          HtmlNamespaceJs[framework][:each] % [namespace.to_json, content]
        end

        def html_namespacing_inline_css(relative_path, absolute_path)
          HtmlNamespacing.options[:styles_for].call(relative_path, absolute_path)
        end
      end

      private

      def self.install_rails_all_versions(options)
        if options[:javascript]
          ::ActionView::Base.class_eval do
            attr_writer(:html_namespacing_rendered_paths)
            def html_namespacing_rendered_paths
              @html_namespacing_rendered_paths ||= []
            end
          end
        end

        ::ActionView::Template.class_eval do
          def render_with_html_namespacing(*args, &block)
            html = render_without_html_namespacing(*args, &block)

            view = args.first

            if view.respond_to?(:html_namespacing_rendered_paths)
              view.html_namespacing_rendered_paths << html_namespacing_key
            end

            if HtmlNamespacing::Plugin::Rails.template_formats.include?(html_namespacing_format)
              add_namespace_to_html(html, view)
            else
              html
            end
          end
          alias_method_chain :render, :html_namespacing

          private

          def add_namespace_to_html(html, view)
            ret = begin
              HtmlNamespacing::add_namespace_to_html(html, html_namespace)
            rescue ArgumentError => e
              HtmlNamespacing::Plugin::Rails.handle_exception(e, self, view)
              html # unless handle_exception() raised something
            end
            ret.respond_to?(:html_safe) && ret.html_safe || ret
          end

          def html_namespace
            HtmlNamespacing::Plugin::Rails.path_to_namespace(html_namespacing_key)
          end
        end
      end

      def self.install_rails_3(options = {})
        self.install_rails_all_versions(options)

        ::ActionView::Template.class_eval do
          private

          def html_namespacing_key
            virtual_path
          end

          def html_namespacing_format
            formats.first
          end
        end
      end

      def self.install_rails_2_3(options = {})
        self.install_rails_all_versions(options)

        ::ActionView::Template.class_eval do
          private

          def html_namespacing_key
            path_without_format_and_extension
          end

          def html_namespacing_format
            format.to_sym
          end
        end
      end
    end
  end
end
