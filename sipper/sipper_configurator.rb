require 'yaml'

class SipperConfigurator
  @@comment_str = <<-EOF
  # Paths that can be set are 
  #
  # :LogPath:
  #     Here is where Sipper creates the log files, default is 
  #     log directory under the project directory
  #
  # :ConfigPath:
  #     This is where the Sipper reads the configuration files from,
  #     the default is config directory
  #
  # :ControllerPath:
  #     The place where sipper loads the controllers, the default is
  #     the controllers directory under project.
  #
  # :SessionRecordPath:
  #     Place where transient session recording takes place. Default is
  #     same as the log directory.
  #
  # :ControllerLibPath:
  #     This is where you may keep any libraries specific to controller
  #     (defaults to controller/lib but can be anything if set)
  #
  # :PStorePath:
  #     This is where PsSipperMap class will store the data, the default
  #     is same as the log location. 
  # 
  # Configuration values are -
  #
  # :LocalSipperIP:  
  #     IP (or name) where the default local Sipper 
  #     instance is going to run. This will 
  #     also be used by the local run of tests. Of 
  #     course you can start the Sipper instance on any 
  #     other IP by providing that IP in the Sipper 
  #     initialization. Can be an array if multihomed.
  #
  # :LocalSipperPort:
  #     Port where default Sipper instance will listen
  #     for incoming messages. Can be an array if multihomed.
  #
  # :LocalSipperTransports
  #     Transport types corresponding to the LocalSipper listen
  #     points. Can be array of strings ["udp", "tcp", "tls"] or even "udp_tcp"
  #     if both UDP and TCP are required to be running for the 
  #     coresponding listen point. The order is not important in this
  #     string. You can have "tls_tcp_udp" to have UDP/TCP and TLS 
  #     transports running. 
  # 
  # :LocalTestPort:  
  #     Port where the Sipper that is running the SipTestCase, 
  #     DrivenSipTestCase and all the tests that are derived 
  #     from them is run. By default when running under a 
  #     project this is same as :LocalSipperPort
  # 
  # :DefaultRIP:     
  #     Default remote IP configured for the installation.
  #     You can of course create a bound session to any
  #     IP address you choose at runtime. 
  # 
  # :DefaultRP:      
  #     Default remote port configured for this installation.
  #     You can of course create a bound session to any
  #     port you choose at runtime.
  # 
  # :SessionRecord: 
  #     default false values can be 'msg-info' and 
  #     'msg-debug'
  #
  # :ProtocolCompliance: 
  #     'strict' or 'lax'
  #
  # :NumThreads: 
  #     number of worker threads
  #
  # :SipperRunFor:
  #     number of seconds after which Sipper will automatically exit
  #     this is useful when you fork a Sipper process for some testing
  #     but then do not have a reference to stop it. 
  #
  # :PrintHeapOnExit: 
  #     if defined then sipper on exit prints heap.
  #
  # :WaitSecondsForTestCompletion: 
  #     seconds that the test should wait 
  #     for signaling to end
  # 
  # :TestManagerName:  
  #     name/ip of the test server where the test 
  #     case is running, this is used when
  #     the UAC or UAS is running on a separate 
  #     server and it is required to 
  #     co-ordinate the test completion using the 
  #     completion signaling.
  # 
  # :TestManagerPort:  
  #     Drb port where the test server is 
  #     listening for requests.
  #     Both Manager configuration are not required 
  #     for locally running tests and UAS/UACs.
  #     The test server IP and Port is where the DRb 
  #     server will be running for remote test 
  #     signaling.
  # 
  # :EnableRecordingLock:  
  #     is a boolean which if set makes use of file locking 
  #     mechanism to synchronize between recording writing 
  #     and reading. If your tests and
  #     controllers are local then with this you can make 
  #     sure that recordings are written first before being 
  #     read. This is anyway an option if you 
  #     are not using DrivenSipTestCase and 
  #     SipTestDriverController, which do this synchronization 
  #     using distributed locking and conditional variables.
  #                       
  # :TimerGranularity:  
  #     the ms value of platform timer granularity, default 
  #     is 50ms. This is the maximum error +/- that shall be 
  #     there in any timer. This error could surface
  #     when there are very few sparse timers in the system. 
  #     For a reasonably loaded system the timer granularity 
  #     does not add any significant error. 
  #                    
  # :TransactionTimers:  
  #     a hash of timer values, can have all the base timer 
  #     constants :t1, :t2, :t4 and even
  #     specific timers like :ta (timerA), tb, tc, td etc 
  #     in which case they override their
  #     dependence on t1 etc. eg. {:t1=>400, :ta=>200, :tb=>32000} 
  #     the default values of these
  #     timers of not specified here is taken from transaction.rb
  #                     
  # :SessionTxnUsage:   
  #     a hash of boolean values for each type of transactions. 
  #     The values of the hash can be 
  #     :use_transactions => boolean, :use_ict => boolean etc. 
  #     Specific type of transactions override the generic 
  #     transaction setting. So if the setting is
  #     {:use_transactions => true} then the Session will use 
  #     transactions of all types. if the setting is 
  #     {:use_transactions => false, :use_ict=> true} then the 
  #     Session will use only Invite Client Transactions. 
  #     There is default setting only for :use_transactions and it 
  #     is "true". So in the absence of any configuration the 
  #     Session will use types of transactions. 
  #     Note these individual configuration values can be 
  #     changed on a Session by Session or even 
  #     request by request basis by setting them on the Session 
  #     by the method of same name as 
  #     the setting. [ e.g my_session.use_ict(true) ]
  #                    
  # :SessionTimer:     
  #      the default session invalidation timer. This timer 
  #      is used when the invalidate is 
  #      called without the force parameter, the session is 
  #      actually invalidated after this
  #      time. This configuration is the default value of the 
  #      timer. This can be overridden 
  #      at the controller level or even session level.
  #                   
  # :SessionLimit:     
  #      in the session_being_invalidated_ok_to_proceed?() 
  #      callback the controller can 
  #      return false which will give the session a new lease 
  #      of life for the time that is equal to the current 
  #      value of session timer. This extension has an upper 
  #      limit roughly equal to :SessionLimit. "Roughly" 
  #      because if the session has lived
  #      for time x, the session timer is x' and the 
  #      :SessionLimit is set for y where
  #      y > x but x + x' > y then the session will not 
  #      re-schedule but invalidate. 
  #      In other words if the increment is such that it 
  #      will increase the lifetime beyond
  #      :SessionLimit then it is not re-scheduled. 
  # 
  # :T2xxUsage:       
  #      a boolean which indicates if the UAS session shall 
  #      retransmit the 2xx or not. 
  #      The default value is true.           
  #                
  # :T2xxTimers:      
  #      a hash of three values that can be used to configure 
  #      the behavior of the 2xx retransmission from the UAS. 
  #      The 3 values are :Start, :Cap, :Limit                  
  #      1> :Start is the starting value of the 2xx 
  #         retransmission timer [13.3.1.4] for UAS
  #         If not configured this defaults to T1 constant defined 
  #         in Transaction class. 
  #         This doubles until it reaches :Cap. 
  #      2> :Cap is the valsue at which the doubling of 
  #         :T2xxRetransStart timer value stops. This 
  #         defaults to T2 defined in the Transaction class. 
  #      3> :Limit is is when the UAS shall abandon the 2xx 
  #         retransmissions if the ACK does not come. 
  #         This defaults to 64*T1. 
  #         e.g :T2xxRetrans = {:Start=>100, :Cap=>400, :Limit=>16000}
  #   
  # :T1xxTimers:      
  #      a hash of two values that can be used to configure 
  #      the behavior of the reliable 1xx retransmissions from the UAS. 
  #      The 2 values are :Start and :Limit                  
  #      1> :Start is the starting value of the 1xx 
  #         retransmission timer for UAS
  #         If not configured this defaults to T1 constant defined 
  #         in Transaction class. 
  #         This doubles for each retransmission.  
  #      2> :Limit is is when the UAS shall abandon the 1xx 
  #         retransmissions if the PRACK does not come. 
  #         This defaults to 64*T1. 
  #         e.g :T1xxRetrans = {:Start=>100, :Limit=>16000}
  #
  # :TargetRefreshMethods:  
  #       list(array) of methods that can update the remote target. 
  # 
  # :DialogCreatingMethods: 
  #       list(array) of dialog creating methods in SIP
  #
  #
  # :ShowSessionIdInMessages:
  #       if set adds the session Id of the session from where a 
  #       message is sent out, in the message as a special header
  #       P-Sipper-Session. 
  # 
  # :PreExistingRouteSet:  
  #       list(array) of Route headers to be used as 
  #       default preloaded route 
  #                          
  # :DigestSalt:    
  #       A configurable secret key for generating 
  #       nonce / cnonce values
  #
  # :SipperMedia:
  #      If set to true the Sipper media library is available
  #      for use.
  #
  # :SipperMediaDefaultControlPort:
  #      The default port where SipperMedia main process
  #      shall listen for control requests
  #
  # :SipperMediaProcessReuse
  #      This flag controls whether to shutdown and launch the SipperMedia if
  #      SipperMedia process is running. Default value is false 
  #
  # :SipperPersistentStore:
  #     Can take value as "file" or "db", in case the value is file
  #     the built in file based persistent store is used while if
  #     the value is db then the configured database is used for 
  #     for persistence needs. The persistent store is used by all 
  #     users of SipperMap API. The default is "file"
  #
  # :SipperPlatformRecordingSeparator:
  #     Platform independent recording separator. Can be CR+LF for windows
  #     while only LF  for Linux. It is set automatically in sipper.rb.
  #
  # :GobletRelease: 
  #     Set to true only if this release is a Goblet release, which is the enhanced 
  #     platform with a number of supporting libraries.
  #
  # :GobletConfigPort:
  #     The port at which Goblet config manager is listening for Goblet config commands
  #     the default value of this port is 4681. Used only if the release is a Goblet
  #     release.
  #
  # :BehindNAT:
  #     If set to true Sipper tries to discover the NAT bindings and makes sure that
  #     it updates the Contact header according to the public IP and port for the UA
  #     It also sends keep alives to maintain the NAT binding during the call
  #
  # :HttpClientThreads:
  #     Number of HTTP Client threads that will be used for asynchronous
  #     http request sending and response processing.
  #
  EOF
  
  def SipperConfigurator.all_keys()
    keys = []
    cmt_str = @@comment_str
    while (x=(cmt_str =~/# :[A-Za-z0-9].*:/)) do
      key =  cmt_str.match(/# :[A-Za-z0-9].*:/)[0]
      keys << key[2..-1]
      cmt_str = cmt_str[x+key.length .. -1]
    end 
    keys
  end
  
  # ignore the first comment as that does not match the key
  def SipperConfigurator.all_comments()
    @@comment_str.split(/# :[A-Za-z0-9].*:/)  
  end
  
  
  
  @@cfg_hash = {}
  
  def SipperConfigurator.[](key)
    @@cfg_hash[key]
  end
  
  def SipperConfigurator.[]=(key, val)
    @@cfg_hash[key] = val
  end
  
  def SipperConfigurator.add_key_description(key, desc)
    unless @@cfg_hash[key.to_sym].nil?
      return false
    end
    @@comment_str << "  # :" << key << ":\n  #     " << desc << "\n  #\n"  
  end
  
  
  def SipperConfigurator.write_yaml_file(file)
    io = File.new(file, "w+")
    io.write(@@comment_str)
    io.write "  # -------------------------------------------------------"
    io.write(YAML::dump(@@cfg_hash)) 
    io.flush
  ensure
    io.close if io
  end
  
  
  def SipperConfigurator.load_yaml_file(file)
    begin
      lsip = lspo = rsip = rspo = nil   
      io = File.new(file, "r")
      @@comment_str = ""
      io.each do |line|
        if line =~ /# ----/
          break
        else  
          @@comment_str << line
        end
      end
      obj = YAML::load(io)
      if obj.class == Hash
        if SipperConfigurator[:CommandlineBitmask]
          lsip = SipperConfigurator[:LocalSipperIP] if ((SipperConfigurator[:CommandlineBitmask] | 8) == SipperConfigurator[:CommandlineBitmask])  
          lspo = SipperConfigurator[:LocalSipperPort] if ((SipperConfigurator[:CommandlineBitmask] | 4) == SipperConfigurator[:CommandlineBitmask])
          rsip = SipperConfigurator[:DefaultRIP] if ((SipperConfigurator[:CommandlineBitmask] | 2) == SipperConfigurator[:CommandlineBitmask])
          rspo = SipperConfigurator[:DefaultRP] if ((SipperConfigurator[:CommandlineBitmask] | 1) == SipperConfigurator[:CommandlineBitmask])
        end
        @@cfg_hash = @@cfg_hash.merge(obj)
        SipperConfigurator[:LocalSipperIP] = lsip if lsip
        SipperConfigurator[:LocalSipperPort] = lspo if lspo
        SipperConfigurator[:DefaultRIP] = rsip if rsip
        SipperConfigurator[:DefaultRP] = rspo if rspo
      else
        msg = "Object read from file #{file} is not a configuration"
        raise TypeError, msg
      end
    rescue IOError
      msg = "#{file} is not a proper file"
      raise TypeError, msg
    ensure
      io.close  if io  
    end 
  end
  
end

# Defaults
SipperConfigurator[:ProtocolCompliance] = 'strict'
SipperConfigurator[:SipperBasePath] = ENV['SIPPER_HOME']
SipperConfigurator[:WaitSecondsForTestCompletion] = 10
SipperConfigurator[:TestManagerName] = nil
SipperConfigurator[:TestManagerPort] = nil
SipperConfigurator[:EnableRecordingLock] = false
SipperConfigurator[:PrintHeapOnExit] = false
SipperConfigurator[:TransactionTimers] = {}
SipperConfigurator[:SessionTxnUsage] = {:use_transactions=>false}  #todo make it true later
SipperConfigurator[:SessionTimer] = 60000 # 50 msec for tests set in driven_sip_test_case, one minute for production
SipperConfigurator[:SessionLimit] = 60000 # one hour default
SipperConfigurator[:T2xxUsage] = false  # todo make it true
SipperConfigurator[:TimerGranularity] = 70  # in milliseconds, default is 50 if not defined
SipperConfigurator[:TargetRefreshMethods] = ['INVITE', 'SUBSCRIBE', 'UPDATE', 'NOTIFY', 'REFER']
SipperConfigurator[:DialogCreatingMethods] = ['INVITE', 'SUBSCRIBE', 'NOTIFY', 'REFER']
SipperConfigurator[:PrintHeapOnExit] = false
SipperConfigurator[:SipperMedia] = false
SipperConfigurator[:SipperMediaDefaultControlPort] = 4680
SipperConfigurator[:SipperMediaProcessReuse] = false
SipperConfigurator[:GobletRelease] = false
SipperConfigurator[:GobletConfigPort] = 4681 if SipperConfigurator[:GobletRelease] 
SipperConfigurator[:CommandlineBitmask] = 0
SipperConfigurator[:HttpClientThreads] = 5
SipperConfigurator[:ShowSessionIdInMessages] = true
SipperConfigurator[:TcpRequestTimeout] = 50000
