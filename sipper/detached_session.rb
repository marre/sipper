require 'session'
require 'sip_logger'

class DetachedSession < Session
  include SipLogger
  
  def initialize(rip, rp, rs, session_limit=nil, sepecified_transport=nil)
    super(nil, nil, nil, rs, session_limit)  
  end
  
end