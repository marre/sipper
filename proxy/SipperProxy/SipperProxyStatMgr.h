#ifndef __SIPPER_PROXY_STAT_MGR_H__
#define __SIPPER_PROXY_STAT_MGR_H__

#include "SipperProxyRawMsg.h"

class SipperProxyStatMgr
{
   private:

      static SipperProxyStatMgr *_instance;

   public:
   
      static SipperProxyStatMgr * getInstance()
      {
         if(_instance == NULL)
         {
            _instance = new SipperProxyStatMgr();
            _instance->_init();
         }

         return _instance;
      }

   private:

      SipperProxyStatMgr() {};
      void _init();

   public:

      void publish(SipperProxyRawMsg *rmsg);
};

#endif
