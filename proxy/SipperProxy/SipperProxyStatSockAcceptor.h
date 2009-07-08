#ifndef __STAT_SOCK_ACCEPTOR_H__
#define __STAT_SOCK_ACCEPTOR_H__

#include "SipperProxyStatMgr.h"

class SipperProxyStatSockAcceptor : public SipperProxyRef
{
   private:

      SipperProxyStatMgr *_mgr;

      static void * _threadStart(void *inData)
      {
         SipperProxyRefObjHolder<SipperProxyStatSockAcceptor> holder((SipperProxyStatSockAcceptor *)inData);

         SipperProxyStatSockAcceptor *obj = holder.getObj();
         obj->_processIncomingConnections();
      }

   public:

      SipperProxyStatSockAcceptor(const std::string &ip, unsigned short port,
                                  SipperProxyStatMgr *mgr) :
         _mgr(mgr)
      {
         if(_openSocket(ip, port) != 0)
         {
            return;
         }

         pthread_t thread;
         addRef();
         pthread_create(&thread, NULL, _threadStart, this);
      }

   private:

      int _openSocket(const std::string &ip, unsigned short port)
      {
         return 0;
      }

      void _processIncomingConnections()
      {
         return;
      }
};

#endif
