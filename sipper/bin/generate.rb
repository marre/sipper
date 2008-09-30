# todo let gems create an executable that can run this from anywhere. 

$:.unshift File.join(File.dirname(__FILE__),"..")

require 'sipper_configurator'
require 'bin/common'

dir = SipperUtil::Common.set_environment()

require "generators/gen_controller"
require "generators/gen_test"

Signal.trap("INT") { puts; exit }


require File.dirname(__FILE__) + '/../version'


def __print_usage
  puts "Options:   --version | -v  => print version information"
  puts "           --help | -h  => print this message"
  puts "           -c|-t [-r] <class_name> <flow_str>"
  puts "         => -c for controller and -t for test generation"
  puts "         => -r to optionally generate the reverse or opposite"
  puts "            flow from flow_str."
  puts "            i.e if flow indicates UAS then generate a UAC etc."
  puts "         => <class_name> is the name of class to be generated, "
  puts "            controller or test."
  puts "         => <flow_str> is the actual call flow." 
  puts "            e.g. '< INVITE, > 100, > 200 {2,7}, < ACK'" 
  exit(0)
end

if %w(--version -v).include? ARGV.first
  puts "generate #{SIP::VERSION::STRING}"
  exit(0)
end

if ((ARGV.length < 3) || %w(--help -h).include?(ARGV.first) || !(%w(-t -c).include?(ARGV.first)))
  __print_usage
end


type = ARGV.shift
reverse = false
if ARGV.first == "-r"
  reverse = true
  ARGV.shift  
end

gcls = ARGV.shift # class name
flow = ARGV.shift.dup

unless gcls && flow 
  __print_usage
end

if reverse
  flow.gsub!("<", "~").gsub!(">", "<").gsub!("~", ">")
end

if type == "-c"
  g = SIP::Generators::GenController.new(gcls, flow)
  g.generate_controller(true, dir.nil? ? nil : File.join(dir, "controllers") )
elsif type == "-t"
  g = SIP::Generators::GenTest.new(gcls, flow)
  g.generate_test(true, dir.nil? ? nil : File.join(dir, "tests"))
else
  __print_usage
end
