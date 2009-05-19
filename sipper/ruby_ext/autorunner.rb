require 'test/unit'
require 'test/unit/autorunner'

module Test
  module Unit
    class AutoRunner
      

      RUNNERS[:report] =  proc do |r|
          require 'ruby_ext/reportrunner'
          Test::Unit::UI::Report::TestRunner
        end

    end
  end
end
