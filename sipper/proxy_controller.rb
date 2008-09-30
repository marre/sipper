# WARNING: This is an experimental proxy controller, to be used for internal Sipper
# UA testing functionality. 
# This is not fully compliant yet. 

require 'b2bua_controller'

module SIP
  class ProxyController < SIP::B2buaController
    
    t2xx_usage false
    
    def self.record_route(val)
      @record_route = val
    end
    
    def self.get_record_route
      @record_route
    end
    
    # Creates the proxy request based on the original request. Also creates the 
    # peer session if it does not already exist. 
    def create_proxy_request(session, orig_request=session.irequest, rip=SipperConfigurator[:DefaultRIP], 
                             rp=SipperConfigurator[:DefaultRP])
      peer_session = get_or_create_peer_session(session, rip, rp)
      if peer_session.initial_state?
        r = peer_session.create_initial_request(orig_request.method, orig_request.uri)
        if self.class.get_record_route
          r.record_route = "<sip:" + SipperConfigurator[:LocalSipperIP]+":"+ SipperConfigurator[:LocalSipperPort].to_s+";lr>"  
        end
      else
        if(orig_request.method == "CANCEL")
          r = peer_session.create_cancel
        elsif(orig_request.method == "ACK")
          r = peer_session.create_ack
        else
          r = peer_session.create_subsequent_request(orig_request.method)
        end  
      end
       
      my_via = r.via
     
      unless (orig_request.method == "CANCEL" || 
       (orig_request.method == "ACK" && peer_session.iresponse.code >= 400))
        r.copy_from(orig_request, :_sipper_all)
        r.push_via(my_via)
        r.format_as_separate_headers_for_mv(:via)
        if (mf_hdr = r[:max_forwards])
          mf = Integer(mf_hdr.to_s)
          mf -= 1
          r.max_forwards = mf.to_s
        else
          r.max_forwards = "70"
        end
      end
      if  r[:route]
        rt = r.route
        if rt.uri.host = SipperConfigurator[:LocalSipperIP] && 
          rt.uri.port == SipperConfigurator[:LocalSipperPort].to_s
          r.pop_route  
        end 
      end
      return r
    end
    
    def create_proxy_response(session, orig_response=session.iresponse)
      peer_session = get_peer_session(session)
      if peer_session
        r = peer_session.create_response(orig_response.code)
        r.copy_from(orig_response,  :_sipper_all)
        r.pop_via
        return r
      else
        raise "Unable to create response"
      end 
    end
    
    def proxy_to(session, host, port)
       get_or_create_peer_session(session, host, port)
       relay_request(session)
    end
    
    # Transparently passes / relays the request given a peer session 
    # exists. 
    def relay_request(session)
      peer_session = get_peer_session(session)
      if peer_session
        r = create_proxy_request(session)
        peer_session.send(r)
      else
        raise "Unable to relay the request" 
      end
    end
    
    # Transparently passes / relays the response given a peer session 
    # exists. 
    def relay_response(session)
      peer_session = get_peer_session(session)
      if peer_session
        r = create_proxy_response(session)
        peer_session.send(r)
      else
        raise "Unable to relay the response" 
      end
    end
    
    # Marks this b2bua as transparent which allows the relaying of requests and 
    # responses between legs without any controller involvement. 
    def go_transparent(session, state=true)
      session[:_sipper_proxy_transparent] = state
      peer_session = get_peer_session(session)
      peer_session[:_sipper_proxy_transparent] = state if peer_session
    end
    
    
    # if there exists a peer session then we transparently send the request as a b2bua
    # if transparent flag is on. 
    def on_request(session)
      peer_session = get_peer_session(session)
      if peer_session && session[:_sipper_proxy_transparent]
        relay_request(session)
      else
        super
      end
    end
    
    
    # if there exists a peer session then we transparently pass the response as a b2bua
    # if transparent flag is true 
    def on_response(session)
      peer_session = get_peer_session(session)
      if peer_session && session[:_sipper_proxy_transparent]
        if session.iresponse.code == 100
          tr_hash = self.class.get_transaction_usage
          return if tr_hash[:use_transactions] || tr_hash[:use_ict] 
        end
        relay_response(session)
      else
        super
      end
    end
    
  end
end
