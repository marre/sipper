# todo let gems create an executable that can run this from anywhere. 
require 'fileutils'

$:.unshift File.join(File.dirname(__FILE__),"..")

require 'sipper_configurator'

#Signal.trap("INT") { puts; exit }

require File.dirname(__FILE__) + '/../version'
def __print_usage
  puts " --version | -v  => print version information"
  puts " --help | -h  => print this message"
  puts ""
  puts " [-i <local_ip>] [-p <local_port>] [-r <remote_ip>] [-o remote_port>]  [-c|-t <file_name>]"
  puts "  => -c for controller and -t for test generation"
  puts "  => <file_name> is the name of test or controller class to be run."
  puts " The default local port while running the controller is 5060 and the default remote port is "
  puts " also 5060. While running the test the default local port is 5066, remote is 5060"
  puts " In its simplest usage, you can just start sipper by running it "
  puts " without arguments. The controllers in this case are loaded from "
  puts " the default controller path location. "
  exit(0)
end

if %w(--help -h).include?(ARGV.first)
  __print_usage
end

if %w(--version -v).include? ARGV.first
  puts "sipper #{SIP::VERSION::STRING}"
  exit(0)
end





if ARGV.length == 0
  require 'bin/common'
  require File.join(File.dirname(__FILE__), "..", "run", "run_sipper1")
  RunSipper1.new.run
else
  # [-i <local_ip>] [-p <local_port>] [-r <remote_ip>] [-o remote_port>]  [-c|-t <file_name>]"
  # Adding a bitwise mask to remember what was sent by commandline
  # lip lp rip rp (8 4 2 1)
  p = nil
  ARGV.each_with_index do |arg, i|  
    if arg == "-m" 
      SipperConfigurator[:SipperMediaDefaultControlPort] = Integer(ARGV[i+1])
    elsif arg == "-i" 
      SipperConfigurator[:LocalSipperIP] = ARGV[i+1]
      SipperConfigurator[:CommandlineBitmask] |= 8
    elsif arg == "-p" 
      p = SipperConfigurator[:LocalSipperPort] = Integer(ARGV[i+1])
      SipperConfigurator[:CommandlineBitmask] |= 4
    elsif arg == "-r" 
      SipperConfigurator[:DefaultRIP] = ARGV[i+1]
      SipperConfigurator[:CommandlineBitmask] |= 2
    elsif arg == "-o"
      SipperConfigurator[:DefaultRP] = Integer(ARGV[i+1])
      SipperConfigurator[:CommandlineBitmask] |= 1
    elsif arg == "-rf"
      SipperConfigurator[:SipperRunFor] = Integer(ARGV[i+1])
    elsif arg == "-t"
      SipperConfigurator[:LocalTestPort] = p if p
      f = ARGV[i+1]
      unless $ULOGNAME
        $ULOGNAME = '_'+File.basename(f)[0...-3]
      end
      ARGV.clear # clear the args
      require 'bin/common'
      load f
    elsif arg == "-c"
      unless $ULOGNAME
        $ULOGNAME = '_' + File.basename(ARGV[i+1])[0...-3]
      end
      require 'bin/common'
      require File.join(File.dirname(__FILE__), "..", "run", "run_sipper1")
      r = RunSipper1.new(ARGV[i+1])
      r.run
    end  
  end
end

