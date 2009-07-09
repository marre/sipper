#include "SipperProxyLogger.h"
LOG("ProxyStatFile");
#include "SipperProxyStatSockDispatcher.h"
#include <sstream>

SipperProxyStatSockDispatcher::SipperProxyStatSockDispatcher(
          int accSock, SipperProxyStatMgr *mgr) :
   SipperProxyStatDispatcher(mgr),
   _sock(accSock)
{
   pthread_t thread;
   addRef();
   pthread_create(&thread, NULL, _threadStart, this);
}

SipperProxyStatSockDispatcher::~SipperProxyStatSockDispatcher()
{
   SipperProxyPortable::disconnectSocket(_sock);
}

void SipperProxyStatSockDispatcher::_processData()
{
   while(!_queue.isQueueStopped())
   {
      SipperProxyQueueData inMsg[100];
      int msgCount = _queue.eventDequeueBlk(inMsg, 100, 500, true);

      for(int idx = 0; idx < msgCount; idx++)
      {
         SipperProxyRefObjHolder<SipperProxyRawMsg> holder((SipperProxyRawMsg *) (inMsg[idx].data));
         SipperProxyRawMsg *msg = holder.getObj();
         unsigned int msgLen = 0;
         char *buffer = msg->getBuf(msgLen);
      }
   }

   return;
}
