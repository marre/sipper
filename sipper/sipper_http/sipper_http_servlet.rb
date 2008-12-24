require 'webrick'
require 'stringio'
require 'lib/http_session'
require 'util/locator'
require 'sipper_http/sipper_http_servlet_request_wrapper'


module SipperHttp
  
  class SipperHttpServlet < WEBrick::HTTPServlet::AbstractServlet
    
    def service(req, res)
      http_session = WEBrick::Session.new(req, res, 
                                          {'session_key' => '_sipper_id', 'database_manager'=>WEBrick::Session::MemoryStore}) 
      s = http_session['sipper_session']
      unless s    
        s = SIP::Locator[:Smr].handle_http_req(req, res)
      end
      
      if s
        http_session['sipper_session'] = s
        s.on_http_request(SipperHttp::SipperHttpServletRequestWrapper.new(req, res))
      else 
        SipperHttp::SipperHttpServlet.send_no_match_err(req, res)    
      end 
    end
    
    def self.send_no_match_err(req, res)
      res.status = 502
      res.body = "<HTML>Unable to find matching controller handler in controller for this request</HTML>"
      res['Content-Type'] = "text/html"     
    end
  end
end