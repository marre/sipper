#include "SipperProxyStatMgr.h"
#include "SipperProxyConfig.h"
#include "SipperProxyStatFileDispatcher.h"
#include "SipperProxyStatSockAcceptor.h"
#include <sstream>

SipperProxyStatMgr * SipperProxyStatMgr::_instance = NULL;

void SipperProxyStatMgr::_init()
{
   SipperProxyConfig &config = SipperProxyConfig::getInstance();

   unsigned int numOutFile = atoi(config.getConfig("StatCollector", "NumOutFile", "1").c_str());

   for(unsigned int idx = 0; idx < numOutFile; idx++)
   {
      std::ostringstream tmp;
      tmp << "Outfile" << idx;
      std::string outfile = config.getConfig("StatCollector", tmp.str(), "");
      tmp.str("");
      tmp << "RollOverSize" << idx;
      std::string rstr = config.getConfig("StatCollector", tmp.str(), "50000000");
      unsigned int rolloverSize = atoi(rstr.c_str());

      if(outfile != "")
      {
         SipperProxyRefObjHolder<SipperProxyStatDispatcher> holder(new SipperProxyStatFileDispatcher(outfile, rolloverSize, this));
      }
   }

   std::string listenPort = config.getConfig("StatCollector", "ListenPort", "0");
   std::string listenIP = config.getConfig("StatCollector", "ListenIp", "127.0.0.1");
   unsigned short port = (unsigned short) atoi(listenPort.c_str());

   if(port != 0)
   {
      SipperProxyRefObjHolder<SipperProxyStatSockAcceptor> holder(new SipperProxyStatSockAcceptor(listenIP, port, this));
   }
}
