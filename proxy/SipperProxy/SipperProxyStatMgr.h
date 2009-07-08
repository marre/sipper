#ifndef __SIPPER_PROXY_STAT_MGR_H__
#define __SIPPER_PROXY_STAT_MGR_H__

#include "SipperProxyRawMsg.h"
#include "SipperProxyLock.h"

class SipperProxyStatMgr;
class SipperProxyStatDispatcher : public SipperProxyRef
{
   protected:

      SipperProxyQueue _queue;
      SipperProxyStatMgr *_mgr;

   private:

      static void _queueMsgCleanup(SipperProxyQueueData indata)
      {
         SipperProxyRawMsg *rmsg = (SipperProxyRawMsg *)indata.data;
         rmsg->removeRef();
      }

   public:

      SipperProxyStatDispatcher(SipperProxyStatMgr *mgr) :
         _queue(false, SIPPER_PROXY_PRELOAD_MSG, SIPPER_PROXY_PRELOAD_MSG),
         _mgr(mgr)
      {
         _queue.registerCleanupFunc(_queueMsgCleanup);
      }

      void loadMessage(SipperProxyRawMsg *rmsg)
      {
         rmsg->addRef();
         SipperProxyQueueData queueMsg;
         queueMsg.data = rmsg;
         _queue.eventEnqueue(&queueMsg);
      }
};

#define MAX_MSG_DISPATHERS 5
class SipperProxyStatMgr
{
   private:

      static SipperProxyStatMgr *_instance;
      SipperProxyMutex _mutex;

      SipperProxyStatDispatcher *_dispatchers[MAX_MSG_DISPATHERS];

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

      SipperProxyStatMgr() 
      {
         for(int idx = 0; idx < MAX_MSG_DISPATHERS; idx++)
         {
            _dispatchers[idx] = NULL;
         }
      };

      void _init();

   public:

      int addDispatcher(SipperProxyStatDispatcher *dispatcher)
      {
         MutexGuard(&_mutex);
         for(int idx = 0; idx < MAX_MSG_DISPATHERS; idx++)
         {
            if(_dispatchers[idx] == dispatcher) return 0;
         }

         for(int idx = 0; idx < MAX_MSG_DISPATHERS; idx++)
         {
            if(_dispatchers[idx] == NULL) 
            {
               dispatcher->addRef();
               return 0;
            }
         }

         return -1;
      }

      void removeDispathcer(SipperProxyStatDispatcher *dispatcher)
      {
         MutexGuard(&_mutex);
         for(int idx = 0; idx < MAX_MSG_DISPATHERS; idx++)
         {
            if(_dispatchers[idx] == dispatcher) 
            {
               dispatcher->removeRef();
               _dispatchers[idx] = NULL;
            }
         }
      }

      void publish(SipperProxyRawMsg *rmsg)
      {
         MutexGuard(&_mutex);
         for(int idx = 0; idx < MAX_MSG_DISPATHERS; idx++)
         {
            if(_dispatchers[idx] != NULL) 
            {
               _dispatchers[idx]->loadMessage(rmsg);
            }
         }
      }
};

#endif
