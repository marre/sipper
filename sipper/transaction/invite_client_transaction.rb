require 'transaction/Ict_sm'
require 'transaction/transaction'
require 'transaction/state_machine_wrapper'
require 'util/timer/sip_timer_helper'
require 'sip_logger'
require 'util/locator'
require 'ruby_ext/object'
require 'request'
require 'util/sipper_util'

module SIP
  module Transaction
    class InviteClientTransaction < SIP::Transaction::BaseTransaction
      include SipLogger
      
      # to be used for accessing msg for printing/recording
      attr_accessor :msg_sent, :cancel_ctxn
      
      # The transport to use, transport flags, remote ip and port
      # the block is to be used to override the T1, T2, even TimerA etc. timer value. 
      # The block can for example have { self.t1 = 200; self.ta=100 } to override the 
      # T1, A on a transaction by transcation basis, otherwise the default is 
      # taken from SipperConfigurator or SIP::Transaction::BaseTransaction::T1
      # in that order. 
      # Usage: TU creates transaction and starts invoking "Invocation Action" 
      # methods. Also for responses received, checks whether to consume them 
      # or not based on consume?  method return value.
      # There are four call backs into TU from ICT and they are notification on 
      # transport error and transaction timeout. 
      # There are 4 TU callbacks. 
      #   (a) transaction_transport_err() gets called should ICT get a transport failure 
      #   (b) transaction_timeout() gets called on timeout of transaction
      #   (c) transaction_transaction_cleanup() gets called on transaction termination.
      #   (d) transaction_wrong_state() if the transaction transition is tried in a wrong state
      #       i.e a message is either received or being tried to send out in a state where it is
      #       illegal to do so. 
      def initialize(tu, branch_id, txn_handler, transport, tp_flags, &block)
        @transaction_name = :Ict # need to have this name defined for every transaction
        @tu = tu
        @branch_id = branch_id
        @transport = transport
        @txn_handler = txn_handler
        @tp_flags = tp_flags
        self.td = 0 if @transport.reliable?  # one line later it can be overidden by arguments.
        super(&block)  # override timers
        #@sm = Ict_sm.new(self)
        SIP::Transaction::StateMachineWrapper.bootstrap_machine(self, Ict_sm)
        @sm = SIP::Transaction::StateMachineWrapper.new(self, @txn_handler, Ict_sm)
        logd("Created the Invite Client Transcation with #{@transport}")
      end
      
      
      def cancel_ctxn=(ctxn)
        @cancel_ctxn = ctxn
        @ok_to_run_timerY = true
        @sm.cancel_sent
      end
      
      
      # Need to handle this after the transition has happened and not in the 
      # _send_to_transport exception handler because state transition from 
      # within a transition is not possible in smc
      def _check_transport_err
        @sm.transport_err if @transport_err
      end
      
      
      #--- SM Invocation Actions <start>---------
      
      #------------------
      # The two methods txn_send and txn_received are two general purpose methods
      # that will be called by the TU as against the transcation type specific 
      # methods defined on each type of transaction. 
      #
      # ICT sends only INVITEs from TU
      def txn_send(msg)
        @message = msg  # the msg is accessed from transaction
        raise "ICT can only send INVITEs" unless (msg.is_request? && msg.method=="INVITE") 
        self._send_invite(msg)
      end
      
      # The ICT receives responses (provisional, success final, non-succes final)
      def txn_received(msg)
        @message = msg
        raise "ICT can only receive responses" unless msg.is_response?
        msg.set_request(@invite)
        case msg.code
        when 100..199
          _provisional_received
        when 200..299
          _success_final_received(msg)
        when 300..699
          _non_success_final_received(msg)
        else
          raise "ICT cannot deal with response #{msg}"
        end    
      end
      #------------------ 
      
      # These method is to be invoked by the TU to send the invite
      # The state machine drives the whole interaction by calling the
      # callbacks. 
      
      def _send_invite(req)
        @invite = req
        @ok_to_run_timerA = true unless @transport.reliable?
        @ok_to_run_timerB = true
        @sm.invite
        _check_transport_err
      rescue Statemap::TransitionUndefinedException => e
        loge "Cannot send invite in #{self.state} state for #{self}"
        @tu.transaction_wrong_state(self) if @tu
        @txn_handler.wrong_state(self) if @txn_handler && @txn_handler.respond_to?(:wrong_state)
      end
      
       
      def _provisional_received
        @sm.provisional
        _check_transport_err
      rescue Statemap::TransitionUndefinedException => e
        loge "Received a provisional in #{self.state} state for #{self}"
        @tu.transaction_wrong_state(self) if @tu
        @txn_handler.wrong_state(self) if @txn_handler && @txn_handler.respond_to?(:wrong_state)
      end
      
      
      # invoked by TU for a 300-699 class response
      def _non_success_final_received(res)
        @response = res
        @sm.non_success_final
        _check_transport_err
      rescue Statemap::TransitionUndefinedException => e
        loge "Received an non 2xx response in #{self.state} state for #{self}"
        @tu.transaction_wrong_state(self) if @tu
        @txn_handler.wrong_state(self) if @txn_handler && @txn_handler.respond_to?(:wrong_state)
      end
      
      def _success_final_received(res)
        @response = res
        @sm.success_final
        _check_transport_err
      rescue Statemap::TransitionUndefinedException => e
        loge "Received a 2xx response in #{self.state} state for #{self}"
        @tu.transaction_wrong_state(self) if @tu
        @txn_handler.wrong_state(self) if @txn_handler && @txn_handler.respond_to?(:wrong_state)
      end
      
      #--- SM Invocation Actions <end>---------
      
      
      #------ Timer callback------
      def on_timer_expiration(timer_task)
        super  # check for invalidation
        case timer_task.tid
        when :ta
          @sm.timer_A(timer_task.duration) if @ok_to_run_timerA
          _check_transport_err
        when :tb
          @sm.timer_B  if @ok_to_run_timerB
        when :td
          @sm.timer_D
        when :ty
          @sm.timer_Y if @ok_to_run_timerY
        end
      rescue Statemap::TransitionUndefinedException => e
        loge "Timer #{timer_task.tid} got fired for #{self}"
        @tu.transaction_wrong_state(self) if @tu
        @txn_handler.wrong_state(self) if @txn_handler && @txn_handler.respond_to?(:wrong_state)
      end
      #----------------------------
      
      
     
      #------SM Callbacks <start>--------
      
      def __send_invite
        logd "Sending INVITE from the Ict #{self}"
        _send_to_transport(@invite) 
      end
      
      def __start_A
        return unless @ok_to_run_timerA
        task = SIP::Locator[:Sth].schedule_for(self, :ta, nil, :transaction, self.ta)
        logd("Starting Timer A #{task} from Ict #{self}")
      end
      
      def __start_B
        return unless @ok_to_run_timerB
        task = SIP::Locator[:Sth].schedule_for(self, :tb, nil, :transaction, self.tb)
        logd("Starting Timer B #{task} from Ict #{self}")
      end
      
      def __start_D
        task = SIP::Locator[:Sth].schedule_for(self, :td, nil, :transaction, self.td)
        logd("Starting Timer D #{task} from Ict #{self}")
      end
      
      def __start_Y
        return unless @ok_to_run_timerY
        task = SIP::Locator[:Sth].schedule_for(self, :ty, nil, :transaction, self.ty)
        logd("Starting Timer Y #{task} from Ict #{self}")
      end
      
      def __reset_A(t)
        self.ta = 2*t
        __start_A
      end
      
      def __timeout
        logw("Transaction timeout happened")
        @txn_handler.timeout(self) if @txn_handler && @txn_handler.respond_to?(:timeout)
        @tu.transaction_timeout(self) if @tu
      end
      
      def __cancel_A
        logd "canceling timer A"
        @ok_to_run_timerA = false
      end
      
      def __cancel_B
        logd "canceling timer B"
        @ok_to_run_timerB = false
      end
      
      def __cancel_Y
        logd "canceling timer Y"
        @ok_to_run_timerY = false
      end
      
      def __consume_msg(b)
        @consume = b
      end
      
      # see section 17.1.1.3 of RFC 3261
      # this is ACK only for non 2xx final responses. 
      def __create_ack()
        return if @ack
        logd("Creating a ACK request for txn #{self}")
        @ack = @tu.create_non_2xx_ack(@invite, @response)  # tu is Session
        logd "The ACK for the Txn #{self} is #{@ack}"
      end 
      
      def __send_ack()
        logd "Sending ACK from the Ict"
        if @ack
          _send_to_transport(@ack)  
        else
          loge "Cannot send ACK from #{self} as no ACK is created so far"
        end
      end
      
      def __transport_err
        loge "A transport error was encountered for #{self} calling TU"
        @txn_handler.transport_err(self) if @txn_handler && @txn_handler.respond_to?(:transport_err)
        @tu.transaction_transport_err(self) if @tu
      end
      
      def __cleanup
        logd "Cleanup called for #{self}"
        @tu.transaction_cleanup(self) if @tu
        @txn_handler.cleanup(self) if @txn_handler && @txn_handler.respond_to?(:cleanup)
      end
      #------SM Callbacks <end>--------
      
      # mask callbacks defined for SM (see Transaction.rb)
      mask_callbacks
      
      private   :_send_to_transport, :_check_transport_err
      protected :_send_invite, :_provisional_received,
                :_success_final_received, :_non_success_final_received
      
    end
  end
end
