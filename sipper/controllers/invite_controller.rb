require 'base_controller'

class InviteController < SIP::BaseController

   transaction_usage :use_transactions=>true
   
   def initialize
     logd("#{name} controller created")
   end
   
   
   def on_invite(session)
     logd("on_invite called for #{name}")
     r = session.create_response(200, "OK")
     r.server = "Sipper"
     session.send(r)
   end
   
   
   def on_ack(session)
     logd("on_ack called for #{name}")
   end
   
   def on_bye(session)
     logd("on_bye called for #{name}, now invalidating")
     r = session.create_response(200, "OK")
     session.send(r)
     session.invalidate(true)
   end
end
