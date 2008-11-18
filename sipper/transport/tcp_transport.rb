
require 'thread'
require 'monitor'
require 'util/message_fill'
require 'sip_logger'
require 'ruby_ext/object'
require 'transport/rel_unrel'
require 'transport/base_transport'
require 'message'
require 'timeout'
require 'util/sipper_util'

module Transport
  class TcpTransport < BaseTransport
    
    include SipLogger
    include SIP::Transport::ReliableTransport 
    
    private_class_method :new
    
    attr_accessor :queue
    
    MAX_RECV_BUFFER = 1024*4
    
    CR   = "\x0d"
    LF   = "\x0a"
    CRLF = "\x0d\x0a"
    @@instance = {}
    @@class_lock = Monitor.new
    
    # comment : need to synchronize on queue and running paramaters
    
    
    def initialize(ip, port, external_q)
      @tid = "TCP"
      if external_q
        @queue = external_q
      else 
        @queue = Queue.new 
      end
      @ip = ip
      @port = port
      @running = false
      @running_lock = Monitor.new 
      logi("Created a new tcp transport with #{@ip} and #{@port}")
    end
    
    
    def to_s
      @str = "tcptransport ip=#{@ip}, port=#{@port}" unless @str
      @str
    end
    
    def TcpTransport.instance(i, p, q=nil)
      @@class_lock.synchronize do 
        k = i.to_s + ":" + p.to_s
        @@instance[k] = new(i, p, q) unless @@instance[k]
        @@instance[k]  
      end
    end
    
    
    def start_transport
      logi("Starting the tcp transport #{self}")
      #@running_lock.synchronize do
      fail "Already running" if @running
      @running = true
      #end
      t = Thread.new do
        Thread.current[:name] = "TCPThread-"+@ip.to_s+"-"+@port.to_s
        TCPSocket.do_not_reverse_lookup = true
        serv = TCPServer.new(@ip.to_s, @port)
        begin 
          loop do       
            begin
              #sock = serv.accept_nonblock
              sock = serv.accept
            rescue Errno::EAGAIN, Errno::ECONNABORTED, Errno::EINTR, Errno::EWOULDBLOCK
              IO.select([sock]) if sock
              retry
            end
            start_sock_thread(sock)
          end  # loop
        rescue  => detail
          logd detail.backtrace.join("\n") 
          loge("Exception #{detail} occured for #{self}")
          break
        end  # exception
      end 
      @t_thread = t
      return t
    end 
    
    def start_sock_thread(sock)
      Thread.start{
        begin
          begin
            addr = sock.peeraddr
            logd "accept: #{addr[3]}:#{addr[1]}"
          rescue SocketError
            logd "accept: <address unknown>"
            raise
          end
          run(sock)
        rescue Errno::ENOTCONN
          logd "Errno::ENOTCONN raised"
        rescue Exception => ex
          loge "Exception2 raised "+ex
          logd ex.backtrace.join("\n") 
        end
      }
    end
    
    def run(sock)
      while true 
        begin
          timeout = SipperConfigurator[:TcpRequestTimeout]||500
          while timeout > 0
            break if IO.select([sock], nil, nil, 0.5)
            timeout = 0 unless @running
            timeout -= 0.5
          end
          if timeout <= 0
            sock.close
            return
          end
          raw_mesg_arr = Array.new
          while line = self.rd_line(sock)
            break if /\A(#{CRLF}|#{LF})\z/om =~ line
            raw_mesg_arr << line
            if line =~ /content-length/i
              l = line.strip
              st = l =~ /\d/
              en = l =~ /\d$/ if st
              cl = line[st..en] if st && en
            end  
          end 
          log_and_raise "No Content-Length." unless cl
          body = ""
          block = Proc.new{|chunk| body << chunk }
          
          remaining_size = cl.to_i
          while remaining_size > 0 
            sz = MAX_RECV_BUFFER < remaining_size ? MAX_RECV_BUFFER : remaining_size
            break unless buf = read_data(sock, sz)
            remaining_size -= buf.size
            block.call(buf)
          end
          if remaining_size > 0 && sock.eof?
            log_and_raise "invalid body size."
          end
          msg = raw_mesg_arr.join(LF)
          msg << body
          # [msg, ["AF_INET", 49361, "ashirs-PC", "127.0.0.1"]]
          mesg = [msg, sock.peeraddr]  
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
            mesg << [@ip, @port, @tid, sock]
            #["msg", ["AF_INET", 33302, "localhost.localdomain", "127.0.0.1"], [our_ip, our_port, TCP, socket]]
            @queue << mesg
            #logi("Message recvd on transport and enqueued on #{@queue}")
          else
            logi("Message recvd on transport does not have the payload or is consumed by a filter, not enquing")
          end     
        end
      end
    end
    
    
    
    def _read_data(io, method, arg)
      begin
        timeout(SipperConfigurator[:TcpRequestTimeout]){
          return io.__send__(method, arg)
        }
      rescue Errno::ECONNRESET
        return nil
      rescue TimeoutError
        log_and_raise "TCP request timeout"
      end
    end
    
    def rd_line(io)
      return _read_data(io, :gets, LF)
    end
    
    def read_data(io, size)
      _read_data(io, :read, size)
    end
    
    def stop_transport
      logi("Stopping transport #{self}")
      #@running_lock.synchronize do
      fail "Already stopped" unless @running
      @running = false
      #end
      @t_thread.kill
      k = @ip.to_s + ":" + @port.to_s
      @@instance[k] = nil
    end
    
    
    def send(mesg, flags, rip, rp, sock)
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
      logsip("O", rip, rp, @ip, @port, @tid, smesg)
      if smesg  
        unless sock  
          sock = TCPSocket.new(rip, rp, @ip, @port)
          start_sock_thread(sock) # start listening as well
        end
        sock.send(smesg)
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

=begin 
t = Transport::TcpTransport.instance("127.0.0.1", 2224)
th = t.start_transport
th.join(10)
p t.queue
puts t.queue.length

if t.queue.length > 0
  m = t.queue.pop
  if m
    puts m[0]
    puts "----------------"
    puts m[1]
    puts "----------------"
    puts m[2]
    puts "----------------"
  end
end
=end