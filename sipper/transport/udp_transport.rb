require 'thread'
require 'monitor'
require 'util/message_fill'
require 'sip_logger'
require 'ruby_ext/object'
require 'transport/rel_unrel'
require 'transport/base_transport'
require 'message'

# todo have a base transport class and also have module Transport
module Transport
  class UdpTransport < BaseTransport
  
    include SipLogger
    include SIP::Transport::UnreliableTransport  # returns false for reliable? check.
    
    private_class_method :new
    
    MAX_RECV_BUFFER = 65500
      
    @@instance = {}
    @@class_lock = Monitor.new
    
    # comment : need to synchronize on queue and running paramaters
    
    
    def initialize(ip, port, external_q)
      @tid = "UDP"
      if external_q
        @queue = external_q
      else 
        @queue = Queue.new 
      end
      @ip = ip
      @port = port
      @running = false
      @running_lock = Monitor.new 
      logi("Created a new udptransport with #{@ip} and #{@port}")
    end
    
    
    def to_s
      @str = "udptransport ip=#{@ip}, port=#{@port}" unless @str
      @str
    end
    
    def UdpTransport.instance(i, p, q=nil)
      @@class_lock.synchronize do 
        k = i.to_s + ":" + p.to_s
        @@instance[k] = new(i, p, q) unless @@instance[k]
        @@instance[k]  
      end
    end
    
    
    def start_transport
      logi("Starting the transport #{self}")
      #@running_lock.synchronize do
        fail "Already running" if @running
        @running = true
      #end
      t = Thread.new do
        Thread.current[:name] = "UDPThread-"+@ip.to_s+"-"+@port.to_s
        UDPSocket.do_not_reverse_lookup = true
        @sock = UDPSocket.new
        @sock.bind(@ip, @port)
        logd "binded ..#{@port}"
        begin 
          loop do
            #puts "starting the select loop.."
            IO.select([@sock])
            mesg = @sock.recvfrom_nonblock(MAX_RECV_BUFFER)
            BaseTransport.in_filters.each do |in_filter|
              logd("Ingress filter applied is #{in_filter.class.name}")
              mesg[0] = in_filter.do_filter(mesg[0])
              break unless mesg[0]
            end
            #["msg", ["AF_INET", 33302, "localhost.localdomain", "127.0.0.1"]]
            if logger.debug?
              logd("Message received is - ")
              mesg.each_with_index {|x,i| logd(" > mesg[#{i}]=#{x}")}    
            end
            if mesg[0]
              mesg << [@ip, @port, @tid]
              @queue << mesg
              #logi("Message recvd on transport and enqueued on #{@queue}")
            else
              logi("Message recvd on transport does not have the payload or is consumed by a filter, not enquing")
            end
            break if mesg[0] =~ /poison dart/
          end  # loop
          rescue  => detail
            logd detail.backtrace.join("\n") 
            loge("Exception #{detail} occured for #{self}")
            @sock.close
            break
        end  # exception
      end 
      @t_thread = t
      return t
    end 
    
    def stop_transport
      logi("Stopping transport #{self}")
      #@running_lock.synchronize do
        fail "Already stopped" unless @running
        @running = false
      #end
      @sock.close
      @t_thread.kill
      k = @ip.to_s + ":" + @port.to_s
      @@instance[k] = nil
    end
    
   
    def send(mesg, flags, *ipport)
      if mesg.class <= ::Message 
        smesg = mesg.to_s
      else
        smesg = mesg
      end 
      logi("Sending message #{smesg} using #{self} to ip=#{ipport[0]} and port=#{ipport[1]}")
      if smesg =~ /_PH_/
        logd("Now filling in message of class #{smesg.class}")
        SipperUtil::MessageFill.sub(smesg, :trans=>@tid, :lip=>@ip, :lp=>@port.to_s) 
      else
        logd("Nothing to fill in message of class #{smesg.class}")
      end
      
      BaseTransport.out_filters.each do |out_filter|
        logd("Outgress filter applied is #{out_filter.class.name}")
        smesg = out_filter.do_filter(smesg)
        break unless smesg
      end
      logsip("O", ipport[0], ipport[1], @ip, @port, @tid, smesg)
      if smesg
        @sock.send(smesg, flags, *ipport)
      else
        logi("Not sending the message as it has probably been nilled out by a filter")
      end
      smesg  # returns for recorder etc.
    end
    
    def get_next_message
      @queue.pop
    end
    
    
    def running
      @running_lock.synchronize do
        return @running
      end
    end
    
  end
end