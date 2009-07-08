#ifndef __STAT_FILE_DISPATCHER_H__
#define __STAT_FILE_DISPATCHER_H__

#include "SipperProxyStatMgr.h"

class SipperProxyStatFileDispatcher : public SipperProxyStatDispatcher
{
   private:

      static void * _threadStart(void *inData)
      {
         SipperProxyRefObjHolder<SipperProxyStatFileDispatcher> holder((SipperProxyStatFileDispatcher *)inData);

         SipperProxyStatFileDispatcher *obj = holder.getObj();
         obj->_mgr->addDispatcher(obj);
         obj->_processData();
         obj->_mgr->removeDispathcer(obj);
      }

   public:

      SipperProxyStatFileDispatcher(const std::string &file, 
                                    SipperProxyStatMgr *mgr) :
         SipperProxyStatDispatcher(mgr)
      {
         if(_openFile(file) != 0)
         {
            return;
         }

         pthread_t thread;
         addRef();
         pthread_create(&thread, NULL, _threadStart, this);
      }

   private:

      int _openFile(const std::string &inFile)
      {
         return 0;
      }

      void _processData()
      {
         return;
      }
};

#endif
