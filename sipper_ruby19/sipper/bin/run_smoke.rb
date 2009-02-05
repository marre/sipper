
$:.unshift File.join(File.dirname(__FILE__), "..", "..","sipper_test")

Signal.trap("INT") { puts; exit }

require File.dirname(__FILE__) + '/../version'

require File.join(File.dirname(__FILE__), "..", "..","sipper_test", "ts_smoke.rb")