# Generated by jeweler
# DO NOT EDIT THIS FILE DIRECTLY
# Instead, edit Jeweler::Tasks in Rakefile, and run the gemspec command
# -*- encoding: utf-8 -*-

Gem::Specification.new do |s|
  s.name = %q{html_namespacing}
  s.version = "0.3.1"

  s.required_rubygems_version = Gem::Requirement.new(">= 0") if s.respond_to? :required_rubygems_version=
  s.authors = ["adamh"]
  s.date = %q{2010-09-02}
  s.description = %q{Inserts "class=" attributes within snippets of HTML so CSS and JavaScript can use automatic scopes}
  s.email = %q{adam@adamhooper.com}
  s.extensions = ["ext/html_namespacing/extconf.rb"]
  s.extra_rdoc_files = [
    "README.rdoc"
  ]
  s.files = [
    "ext/html_namespacing/html_namespacing.c",
     "ext/html_namespacing/html_namespacing.h",
     "ext/html_namespacing/html_namespacing_ext.c",
     "lib/html_namespacing.rb",
     "lib/html_namespacing/plugin.rb",
     "lib/html_namespacing/plugin/dom_scan_jquery.js",
     "lib/html_namespacing/plugin/dom_scan_jquery.js.compressed",
     "lib/html_namespacing/plugin/rails.rb",
     "lib/html_namespacing/plugin/sass.rb"
  ]
  s.homepage = %q{http://github.com/adamh/html_namespacing}
  s.rdoc_options = ["--charset=UTF-8"]
  s.require_paths = ["lib"]
  s.rubygems_version = %q{1.3.6}
  s.summary = %q{Automatic HTML namespacing}
  s.test_files = [
    "spec/spec_helper.rb",
     "spec/c_extension_spec.rb"
  ]

  if s.respond_to? :specification_version then
    current_version = Gem::Specification::CURRENT_SPECIFICATION_VERSION
    s.specification_version = 3

    if Gem::Version.new(Gem::RubyGemsVersion) >= Gem::Version.new('1.2.0') then
      s.add_runtime_dependency(%q<glob_fu>, [">= 0.0.4"])
    else
      s.add_dependency(%q<glob_fu>, [">= 0.0.4"])
    end
  else
    s.add_dependency(%q<glob_fu>, [">= 0.0.4"])
  end
end

