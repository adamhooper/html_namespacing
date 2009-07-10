module HtmlNamespacing
  module Plugin
    module Sass
      def self.install(options = {})
        options[:prefix] ||= 'views'
        options[:callback] ||= lambda { |p| HtmlNamespacing::Plugin.default_relative_path_to_namespace(p) }
        Sass_2_2.install(options)
      end

      module Sass_2_2
        def self.install(options = {})
          self.allow_updating_partials
          self.add_super_rule_to_tree_node(options)
          self.try_to_enable_memoization
        end

        private

        def self.allow_updating_partials
          ::Sass::Plugin.module_eval do
            private

            # Though a bit over-zealous, who *cares* if we render partial
            # Sass files?
            def forbid_update?(*args)
              false
            end
          end
        end

        def self.add_super_rule_to_tree_node(namespacing_options)
          ::Sass::Tree::RuleNode.class_eval do
            def to_s_with_namespacing(tabs, super_rules = nil)
              super_rules ||= namespacing_rules
              to_s_without_namespacing(tabs, super_rules)
            end
            alias_method_chain(:to_s, :namespacing)

            private

            define_method(:namespacing_prefix) do
              namespacing_options[:prefix]
            end
            define_method(:namespacing_callback) do
              namespacing_options[:callback]
            end

            def namespacing_regex
              /^#{Regexp.quote(options[:css_location])}\/#{Regexp.quote(namespacing_prefix)}\/(.*)\.css$/
            end

            def namespace
              @namespace ||= if options[:css_filename] =~ namespacing_regex
                namespacing_callback.call($1.split('.')[0])
              end
              @namespace
            end

            def namespacing_rules
              namespace && [ ".#{namespace}" ]
            end
          end
        end

        def self.try_to_enable_memoization
          ::Sass::Tree::RuleNode.class_eval do
            extend ActiveSupport::Memoizable
            memoize :namespacing_rules
          end
        rescue NameError
          # ActiveSupport isn't loaded
        end
      end
    end
  end
end
