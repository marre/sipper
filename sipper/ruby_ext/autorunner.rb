require 'test/unit'
require 'test/unit/autorunner'

module Test
  module Unit
    class AutoRunner
      

      RUNNERS = {
        :console => proc do |r|
          require 'test/unit/ui/console/testrunner'
          Test::Unit::UI::Console::TestRunner
        end,
        :gtk => proc do |r|
          require 'test/unit/ui/gtk/testrunner'
          Test::Unit::UI::GTK::TestRunner
        end,
        :gtk2 => proc do |r|
          require 'test/unit/ui/gtk2/testrunner'
          Test::Unit::UI::GTK2::TestRunner
        end,
        :fox => proc do |r|
          require 'test/unit/ui/fox/testrunner'
          Test::Unit::UI::Fox::TestRunner
        end,
        :tk => proc do |r|
          require 'test/unit/ui/tk/testrunner'
          Test::Unit::UI::Tk::TestRunner
        end,
        :report => proc do |r|
          require 'ruby_ext/reportrunner'
          Test::Unit::UI::Report::TestRunner
        end,
      }

    end
  end
end
