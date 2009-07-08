#ifndef __SIPPER_PROXY_REF_H__
#define __SIPPER_PROXY_REF_H__

#include <pthread.h>

class SipperProxyRef
{
   protected:

      pthread_mutex_t _mutex;
      unsigned int _count;

   protected:

      SipperProxyRef();
      virtual ~SipperProxyRef();

   public:

      void addRef();
      virtual unsigned int removeRef();

   private:

      SipperProxyRef(const SipperProxyRef &);
      SipperProxyRef & operator = (const SipperProxyRef &);
};

class SipperProxyRefObjHolder
{
   private:

      SipperProxyRef *_obj;

   public:

      SipperProxyRefObjHolder(SipperProxyRef *);
      ~SipperProxyRefObjHolder();

      SipperProxyRef * getObj();
      void setObj(SipperProxyRef *inObj);

   private:

      SipperProxyRefObjHolder(SipperProxyRefObjHolder &);
      SipperProxyRefObjHolder & operator = (const SipperProxyRefObjHolder &);
};

#endif
