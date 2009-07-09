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

   private:
      
      FILE *_fp;
      std::string _filename;
      unsigned int _recCount;

      unsigned int _currFileIdx;
      unsigned int _currFileSize;

   public:

      SipperProxyStatFileDispatcher(const std::string &file, 
                                    unsigned int recCount,
                                    SipperProxyStatMgr *mgr);
      ~SipperProxyStatFileDispatcher();

   private:

      int _openFile();
      void _processData();
};

#endif
