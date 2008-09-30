require 'transport/udp_transport'
require 'sip_logger'
require 'util/sipper_util'
require 'message'
require 'request'
require 'response'
require 'session'
require 'udp_session'
require 'util/timer/timer_task'
require 'stray_message_manager'
require 'sipper_http/sipper_http_response'

class SipMessageRouter

  include SipLogger
  include SipperUtil
  
  attr_reader :tg, :running
  
  def initialize(queue, num_threads=5)
    @q = queue
    @num_threads = num_threads
    @tg = ThreadGroup.new
    @running = false  
    @run = false
    # todo running and run is not thread safe 
    log_and_raise "Message Queue is not set", ArgumentError unless @q
  end
  
  
  def start
    @run = true
    1.upto(@num_threads) do |i|
      @tg.add(
        Thread.new do
          Thread.current[:name] = "WorkerThread-"+i.to_s
          while @run
            msg = @q.pop
            #logd("Message #{msg} picked up from queue")
            logi("Message picked from queue, now parsing")
            r = Message.parse(msg)
            2.times do  # one optional retry
              case r
              when Request
                logd("REQUEST RECEIVED #{r}")
                logsip("I", r.rcvd_from_info[3], r.rcvd_from_info[1], r.rcvd_at_info[0], r.rcvd_at_info[1], r)
                if r.to_tag && !r.attributes[:_sipper_initial]
                  # call_id, local, remote
                  s = SessionManager.find_session(r.call_id, r.to_tag, r.from_tag)  
                  if s
                    s.on_message r
                    break
                  else
                    if hndlr = SIP::StrayMessageManager.stray_message_handler
                      logd("Found a stray message handler for the request, invoking it")
                      ret = hndlr.handle(r)
                      case ret[0]
                      when SIP::StrayMessageHandler::SMH_DROP
                        logw("A stray request #{r.method} being dropped by SMH")
                        break
                      when SIP::StrayMessageHandler::SMH_HANDLED
                        logd("A stray request #{r.method} handled by SMH")
                        break
                      when SIP::StrayMessageHandler::SMH_RETRY
                        logd("A stray request #{r.method} received, SMH retries")
                        r = ret[1] if ret[1]
                        if r.attributes[:_sipper_retried]
                          logw("Already retried request now dropping")
                          break
                        else
                          r.attributes[:_sipper_retried] = true
                          next
                        end
                      when SIP::StrayMessageHandler::SMH_TREAT_INITIAL
                        logd("A stray request #{r.method} received, SMH treating as initial")
                        r = ret[1] if ret[1]
                        if r.attributes[:_sipper_initial]
                          logw("Already retried request now dropping")
                          break
                        else
                          r.attributes[:_sipper_initial] = true
                          next
                        end
                      else
                        logw("A stray request #{r.method} SMH response not understood, dropping")
                        break
                      end
                    else
                      logw("A stray request #{r.method} received, dropping as no handler")
                      break
                    end  
                  end
                else
                  # call_id, local, remote
                  s = SessionManager.find_session(r.call_id, r.to_tag, r.from_tag)
                  if s
                    logd("Matched session #{s}")
                    s.on_message r
                    break
                  else  # create a new session
                    logd("No matching session found")
                    ctrs = SIP::Locator[:Cs].get_controllers(r)
                    logd("Initial request, total controllers returned are #{ctrs.size}")
                    ctrs.each do |c| 
                      if c.interested?(r)
                        logd("Controller #{c.name} is interested in #{r.method}")
                        #["AF_INET", 33302, "localhost.localdomain", "127.0.0.1"]
                        # todo tcp support here
                        if stp = c.specified_transport
                          logd("Controller #{c.name} specified #{stp} transport")
                          unless (stp[0] == r.rcvd_at_info[0] &&  
                                  stp[1] == r.rcvd_at_info[1])
                            next        
                          end        
                        end
                        if(r.rcvd_from_info[0] == "AF_INET") # todo need to identify the transport 
                          s = c.create_udp_session(r.rcvd_from_info[3], r.rcvd_from_info[1])
                        end
                        s.pre_set_dialog_id(r)  # to facilitate addition in session manager 
                        SessionManager.add_session(s, false)
                        s.on_message(r)
                        break
                      end
                    end  # controller loop
                    # if no controller on initial request then respond with 502
                    unless s
                      if hndlr = SIP::StrayMessageManager.stray_message_handler
                        logd("Found a stray message handler for the init request, invoking it")
                        ret = hndlr.handle(r) 
                        case ret[0]
                        when SIP::StrayMessageHandler::SMH_DROP
                          logw("A stray init request #{r.method} being dropped by SMH")
                          break
                        when SIP::StrayMessageHandler::SMH_HANDLED
                          logd("A stray init request #{r.method} handled by SMH")
                          break
                        when SIP::StrayMessageHandler::SMH_RETRY
                          logd("A stray init request #{r.method} received, SMH retries")
                          r = ret[1] if ret[1]
                          if r.attributes[:_sipper_retried]
                            logw("Already retried init request now dropping")
                            break
                          else
                            r.attributes[:_sipper_retried] = true
                            next
                          end
                        else
                          logw("A stray init request #{r.method} SMH response not understood, dropping")
                          break
                        end
                      else
                        # todo either send a 502 or cleanup the log
                        logw("A stray init request #{r.method} received, sending 502")
                        break
                      end
                    else
                      break
                    end
                  end  # if session found
                end  # r.to-tag
  
              when Response
                logd("RESPONSE RECEIVED #{r}")
                logsip("I", r.rcvd_from_info[3], r.rcvd_from_info[1],  r.rcvd_at_info[0], r.rcvd_at_info[1], r)
                # call_id, local, remote
                s = SessionManager.find_session(r.call_id, r.from_tag, r.to_tag, (SipperUtil::SUCC_RANGE.include?r.code))
                if s
                  logd("Session found, sending response to session")
                  s.on_message r
                  break
                else
                  if hndlr = SIP::StrayMessageManager.stray_message_handler
                    logd("Found a stray message handler for the response, invoking it")
                    ret = hndlr.handle(r)
                    case ret[0]
                    when SIP::StrayMessageHandler::SMH_DROP
                      logw("A stray response #{r.code} being dropped by SMH")
                      break
                    when SIP::StrayMessageHandler::SMH_HANDLED
                      logd("A stray response #{r.code} handled by SMH")
                      break
                    when SIP::StrayMessageHandler::SMH_RETRY
                      logd("A stray response #{r.code} received, SMH retries")
                      r = ret[1] if ret[1]
                      if r.attributes[:_sipper_retried]
                        logw("Already retried response now dropping")
                        break
                      else
                        r.attributes[:_sipper_retried] = true
                        next
                      end
                    else
                      logw("A stray response #{r.code} SMH response not understood, dropping")
                      break
                    end
                  else
                    logw("A stray response #{r.code} received, dropping")
                    break
                  end
                end
                
              when SIP::TimerTask
                logd("TIMER RECEIVED #{r}")
                r.invoke
                break
              when Media::SipperMediaEvent
                logd("Media Response/Event received")
                r.session.on_message r
                break
              when SipperHttp::SipperHttpResponse
                logd("Sipper HTTP response received")
                r.dispatch
              else
                logw("DONT KNOW WHAT YOU SENT")
                break
              end
            end
          end
        end
      )
    end
    @running = true
  end
  
  def stop
    @run = false
    @running = false
    @tg.list.each { |t| t.kill }
  end
  
end
