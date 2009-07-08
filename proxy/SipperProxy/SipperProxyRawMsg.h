#ifndef __SIPPER_PROXY_RAW_MSG_H__
#define __SIPPER_PROXY_RAW_MSG_H__

#include "SipperProxyRef.h"
#include "SipperProxyMsgFactory.h"

class SipperProxyRawMsg : virtual public SipperProxyRef
{
   private:

      char _defaultMem[SIPPER_PROXY_DEF_MSGLEN];

      char *_defaultBuffer;
      unsigned int _defaultMemLen;
      char *_allocBuffer;
      
   private:

      char *_buffer;
      unsigned int _bufLen;

   protected:

      SipperProxyRawMsg();
      ~SipperProxyRawMsg();

      void _releaseBuf();

   public:

      static SipperProxyRawMsg * getFactoryMsg();
      static void setFactoryLen(unsigned int factoryLen);
      static void closeFactory();

      char * getBuf(unsigned int &len)
      {
         len = _bufLen;
         return _buffer;
      }

      unsigned int getLen() const
      {
         return _bufLen;
      }

      void setData(const char *inBuf, unsigned int inBufLen);
      void setLen(unsigned int inBufLen);
      void reset()
      {
         _releaseBuf();
      }

      virtual unsigned int removeRef();

   public:

      friend class SipperProxyMsgFactory<SipperProxyRawMsg>;

   private:

      bool _facObj;

      static SipperProxyMsgFactory<SipperProxyRawMsg> _factory;

   public:

      static std::string toLog(unsigned int tabCount)
      {
         _factory.setName("SipperProxyRawMsg");
         return _factory.toLog(tabCount);
      }
};

#endif
