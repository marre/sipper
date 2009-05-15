
$:.unshift File.join(File.dirname(__FILE__),"..")

require 'sipper_configurator'
require 'bin/common'
dir = SipperUtil::Common.set_environment()

require File.dirname(__FILE__) + '/../version'

require 'rubygems'


def gem_paths(spec)
  spec.require_paths.collect { |d|
    File.join(spec.full_gem_path,d)
  }
end 
spec = Gem::GemPathSearcher.new.find("sipper")
if spec
  p = (gem_paths(spec))[0]
  p.slice!(/\/sipper$/)
  
  SipperConfigurator[:SipperBasePath] = p if p
  
  SipperConfigurator.write_yaml_file(File.join(SipperConfigurator[:ConfigPath], "sipper.cfg"))
end