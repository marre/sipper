#include "SipperProxyStatMgr.h"
#include "SipperProxyConfig.h"
#include "SipperProxyStatFileDispatcher.h"
#include "SipperProxyStatSockAcceptor.h"

SipperProxyStatMgr * SipperProxyStatMgr::_instance = NULL;

void SipperProxyStatMgr::_init()
{
   SipperProxyConfig &config = SipperProxyConfig::getInstance();

   std::string outfile = config.getConfig("StatCollector", "Outfile", "");

   if(outfile != "")
   {
      SipperProxyRefObjHolder<SipperProxyStatDispatcher> holder(new SipperProxyStatFileDispatcher(outfile, this));
   }

   std::string listenPort = config.getConfig("StatCollector", "ListenPort", "0");
   std::string listenIP = config.getConfig("StatCollector", "ListenIp", "127.0.0.1");
   unsigned short port = (unsigned short) atoi(listenPort.c_str());

   if(port != 0)
   {
      SipperProxyRefObjHolder<SipperProxyStatSockAcceptor> holder(new SipperProxyStatSockAcceptor(listenIP, port, this));
   }
}
