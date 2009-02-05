require 'rubygems' 
require 'sipper/version'

SPEC = Gem::Specification.new do |s| 
   s.name          = "Sipper" 
   s.version       =  "#{SIP::VERSION::STRING}"
   s.author        = "Agnity Inc. Canada" 
   s.email         = "nasir.khan@agnity.com" 
   s.platform      = Gem::Platform::RUBY 
   s.summary       = "Sipper - World's most productive SIP platform" 
   candidates      = Dir.glob("{sipper,sipper_test,Rakefile}/**/*") 
   s.files         = candidates.delete_if do |item| 
                       item.include?(".svn") 
                     end 
   s.require_path  = "sipper" 
   s.autorequire   = "sipper" 
   s.has_rdoc       = true 
   s.add_dependency("facets", "= 1.8.54") 
   s.add_dependency("flexmock", "= 0.7.1") 
   s.add_dependency("log4r", "= 1.0.5")  
end  
