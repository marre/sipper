module SIP
  module VERSION #:nodoc:
    unless defined? MAJOR
      MAJOR = 1
      MINOR = 1
      TINY  = 3
      STRING = [MAJOR, MINOR, TINY].join('.')
    end
  end
end
