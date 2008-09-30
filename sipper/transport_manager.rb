require 'transport/udp_transport'

class TransportManager

  attr_reader :transports
    
  def initialize
    @transports = []
  end
  
  def add_transport tp
    @transports << tp
  end  
  
  #["AF_INET", 33302, "localhost.localdomain", "127.0.0.1"]
  def get_transport_for(transport_info)
    @transports[0]   #todo have the MH logic here
  end
  
  # todo find a way to return a different
  # this return transport suitable for sending to ip / port
  def get_udp_transport_for(ip, port)
    @transports[0]
  end
  
  # This returns a transport that matches the ip and port as given
  def get_udp_transport_with(ip, port)
    @transports.each do |tr|
      return tr if (tr.ip == ip && tr.port == port)  
    end
  end
  
end
