module SIP
  module VERSION #:nodoc:
    unless defined? MAJOR
      MAJOR = 2
      MINOR = 0
      TINY  = 0
      STRING = [MAJOR, MINOR, TINY].join('.')
    end
  end
end
