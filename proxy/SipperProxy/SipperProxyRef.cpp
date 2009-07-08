#include "SipperProxyRef.h"

SipperProxyRef::SipperProxyRef()
{
   pthread_mutex_init(&_mutex, NULL);
   _count = 1;
}

SipperProxyRef::~SipperProxyRef()
{
   pthread_mutex_destroy(&_mutex);
}

void SipperProxyRef::addRef()
{
   pthread_mutex_lock(&_mutex);
   _count++;
   pthread_mutex_unlock(&_mutex);
}

unsigned int SipperProxyRef::removeRef()
{
   pthread_mutex_lock(&_mutex);
   _count--;
   unsigned int locCount = _count;
   pthread_mutex_unlock(&_mutex);

   if(locCount == 0)
   {
      delete this;
   }

   return locCount;
}

SipperProxyRefObjHolder::SipperProxyRefObjHolder(SipperProxyRef *obj)
{
   _obj = obj;
}

SipperProxyRefObjHolder::~SipperProxyRefObjHolder()
{
   if(_obj != NULL)
   {
      _obj->removeRef();
   }
}

SipperProxyRef * SipperProxyRefObjHolder::getObj()
{
   return _obj;
}

void SipperProxyRefObjHolder::setObj(SipperProxyRef *inObj)
{
   if(inObj == _obj)
   {
      return;
   }

   if(_obj != NULL)
   {
      _obj->removeRef();
   }

   _obj = inObj;
}
