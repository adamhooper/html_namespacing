require 'rubygems'
require 'rake'

begin
  require 'jeweler'
  Jeweler::Tasks.new do |gem|
    gem.name = 'html_namespacing'
    gem.summary = 'Automatic HTML namespacing'
    gem.email = 'adam@adamhooper.com'
    gem.homepage = 'http://github.com/adamh/html_namespacing'
    gem.description = 'Inserts "class=" attributes within snippets of HTML so CSS and JavaScript can use automatic scopes'
    gem.authors = ['adamh']
    gem.files = FileList['lib/**/*.rb', 'rails/**/*.rb', 'ext/**/*.[ch]', 'lib/html_namespacing/plugin/dom_scan_*.js*'].to_a
    gem.extensions = ['ext/html_namespacing/extconf.rb']
    gem.add_dependency 'glob_fu', '>= 0.0.4'
  end
  Jeweler::GemcutterTasks.new

rescue LoadError
  puts "Jeweler (or a dependency) not available. Install it with: sudo gem install jeweler"
end

require 'spec/rake/spectask'
Spec::Rake::SpecTask.new(:spec) do |spec|
  spec.libs << 'lib' << 'spec'
  spec.spec_files = FileList['spec/**/*_spec.rb']
end

Spec::Rake::SpecTask.new(:rcov) do |spec|
  spec.libs << 'lib' << 'spec'
  spec.pattern = 'spec/**/*_spec.rb'
  spec.rcov = true
end

task :spec => :check_dependencies

task :default => :spec

require 'rake/rdoctask'
Rake::RDocTask.new do |rdoc|
  if File.exist?('VERSION.yml')
    config = YAML.load(File.read('VERSION.yml'))
    version = "#{config[:major]}.#{config[:minor]}.#{config[:patch]}"
  else
    version = ""
  end

  rdoc.rdoc_dir = 'rdoc'
  rdoc.title = "asset_library #{version}"
  rdoc.rdoc_files.include('README*')
  rdoc.rdoc_files.include('lib/**/*.rb')
end

