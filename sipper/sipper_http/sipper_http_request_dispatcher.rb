require 'net/http'
require 'thread'
require 'uri'
require 'sipper_http/sipper_http_response'
require 'sip_logger'

class HttpUrlContext
  attr_accessor :url, :session, :params, :req_method  
  def initialize(url, req_method, session, params)
    @url = url
    @session = session
    @params = params
    @req_method = req_method
  end
end


class SipperHttpRequestDispatcher
  include SipLogger
  # sipper_message_queue is the main event queue in sipper 
  # this is where all the sip/timer/media and now http responses
  # are queued.
  def initialize(sipper_msg_queue, num_threads = 5)
    @num_threads = num_threads
    @request_queue = Queue.new 
    @run = false
    @smq = sipper_msg_queue
  end
  
  def start
    @run = true
    1.upto(@num_threads) do |i|
      Thread.new do
        Thread.current[:name] = "HttpClientThread-"+i.to_s
        while @run
          url_context = @request_queue.pop
          url = URI.parse(url_context.url)
          res = nil
          if url_context.req_method == 'get'
            req = Net::HTTP::Get.new(url.path)
            res = Net::HTTP.start(url.host, url.port) {|http|
              http.request(req)
            }
          elsif url_context.req_method == 'post'
            req = Net::HTTP::Post.new(url.path)
            req.set_form_data( url_context.params, '&')
            res = Net::HTTP.new(url.host, url.port).start {|http| http.request(req) }                   
          end
          sipper_res = SipperHttp::SipperHttpResponse.new(res, url_context.session)
          @smq << sipper_res
        end  
      end
    end  
  end
  
  def stop
    @run = false
  end
  
  # params are a hash of POST parameters
  def request_post(url, session, params)
    logd('POST request called on dispatcher, now enquing request')
    @request_queue << HttpUrlContext.new(url, 'post', session, params)  
  end
  
  def request_get(url, session)
    logd('GET request called on dispatcher, now enquing request')
    @request_queue << HttpUrlContext.new(url, 'get', session, nil)
  end
  
end
