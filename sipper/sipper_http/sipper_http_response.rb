require 'delegate'

# Wraps the HTTPResponse, with Sipper context data.

module SipperHttp
  class SipperHttpResponse < SimpleDelegator
    
    attr_reader :session, :wrapped_http_res
    
    def initialize(http_res, session)
      super(http_res)  
      @session = session
      @wrapped_http_res = http_res
    end
    
    def dispatch    
      @session.on_http_response(self)  
    end
    
    def short_to_s
      self.inspect  
    end
    
  end
end