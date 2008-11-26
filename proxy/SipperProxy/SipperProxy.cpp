#include "SipperProxyLogger.h"
LOG("ProxyMain");
#include "SipperProxy.h"
#include "SipperProxyConfig.h"
#include "SipperProxyLogMgr.h"

#include <netdb.h>

DnsCache::DnsCache()
{
   _lastCheckTime = time(NULL); 
}

void DnsCache::addEntry(const std::string &name, in_addr_t ip, boolean removable)
{
   DnsEntry entry;
   entry.addr[0] = netIP;
   entry.entryCount = 0;

   if(removable)
   {
      entry.entryTime = time(NULL);
   }
   else
   {
      entry.entryTime = -1;
   }

   _entries[name] = entry;
}

in_addr_t DnsCache::getIp(const std::string &hostname)
{
   in_addr_t ret = inet_addr(hostname.c_str());

   if(ret != -1) return ret;

   DnsMapCIt it = _entries.find(hostname);

   if(it != _entries.end())
   {
      return it->second.addr[0];
   }

   struct hostent *hentry = gethostbyname(hostname.c_str());

   if(hentry == NULL)
   {
      logger.logMsg(ERROR_FLAG, 0, "Error doing DNS query [%s] [%s]\n",
                    strerror(SipperProxyPortable::getErrorCode()),
                    hostname.c_str());
      return 1;
   }

   char **currentry;
   DnsEntry entry;

   for(currentry = hentry->h_addr_list; *currentry != NULL; currentry++)
   {
      if(entry.entryCount == 5) break;

      in_addr_t netIP = 0;
      memcpy(&netIP, *currentry, sizeof(int));
      entry.addr[entry.entryCount] = netIP;

      entry.entryCount++;
   }

   if(entry.entryCount == 0)
   {
      logger.logMsg(ERROR_FLAG, 0, "No records from DNS [%s]\n",
                    hostname.c_str());
      return -1;
   }

   _entries[hostname] = entry;
   return entry.addr[0];
}

void DnsCache::checkCache()
{
   time_t now = time(NULL);
   //Check every hour and clear entries older than 2hrs.
   if((now - _lastCheckTime) < 3600) return;

   _lastCheckTime = now;

   DnsMapIt it = _entries.begin();

   while(it != _entries.end())
   {
      if(it->second.entryTime == -1)
      {
         ++it;
         continue;
      }

      if((now - it->second.entryTime) > 7200)
      {
         _entries.erase(it++);
      }
      else
      {
         ++it;
      }
   }
}

int main(int argc, char **argv)
{
   std::string configFile("SipperProxy.cfg");
   std::string logFile("SipperProxyLog.lcfg");

   for(int idx = 1; (idx + 1) < argc; idx += 2)
   {
      std::string option = argv[idx];
      std::string value = argv[idx + 1];

      if(option == "-c")
      {
        configFile = value;
      }
      else if(option == "-l")
      {
        logFile = value;
      }
   }

   LogMgr::instance().init(logFile.c_str());
   SipperProxyConfig &config = SipperProxyConfig::getInstance();
   config.loadConfigFile(configFile);

   SipperProxy proxy;
   proxy.start();

   logger.logMsg(ALWAYS_FLAG, 0, "SipperProxy program ended.\n");
}

SipperProxy::SipperProxy() :
   _sipperOutSocket(-1),
   _sipperInSocket(-1),
   _inAddr(""),
   _outAddr(""),
   _inPort(0),
   _outPort(0),
   _numOfSipperDomain(0),
   _toSendIndex(0),
   sipperDomains(NULL)
{
   SipperProxyConfig &config = SipperProxyConfig::getInstance();

   _inStrAddr = config.getConfig("Global", "InAddr", "127.0.0.1");
   //_outStrAddr = config.getConfig("Global", "OutAddr", "");

   if(_outStrAddr == "")
   {
      _outStrAddr = _inStrAddr;
   }

   _inPort = (unsigned short) atoi(
                 config.getConfig("Global", "InPort", "5700").c_str());
   //_outPort = (unsigned short) atoi(
                 //config.getConfig("Global", "OutPort", "0").c_str());

   if(_outPort == 0) _outPort = _inPort;

   if((_sipperInSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
      logger.logMsg(ERROR_FLAG, 0, "Socket creation error. [%s]\n",
                    strerror(SipperProxyPortable::getErrorCode()));
      exit(1);
   }

   u_int flagOn = 1;
#ifndef __UNIX__
   setsockopt(_sipperInSocket, SOL_SOCKET, SO_REUSEADDR, 
              (const char *)&flagOn, sizeof(flagOn));
#else
   setsockopt(_sipperInSocket, SOL_SOCKET, SO_REUSEADDR, 
              &flagOn, sizeof(flagOn));
#endif

   _inAddr = new sockaddr_in();
   memset(_inAddr, 0, sizeof(sockaddr_in));

   _inAddr->sin_family = AF_INET;
   _inAddr->sin_addr.s_addr = inet_addr(_inStrAddr.c_str());
   _inAddr->sin_port = htons(_inPort);

   if(bind(_sipperInSocket, (sockaddr *)_inAddr, sizeof(sockaddr_in)) < 0)
   {
      logger.logMsg(ERROR_FLAG, 0,
                    "Error binding port[%d]. [%s]\n", _inPort,
                    strerror(SipperProxyPortable::getErrorCode()));
      exit(1);
   }

   SipperProxyPortable::setNonBlocking(_sipperInSocket);

   if((_inStrAddr == _outStrAddr) && (_inPort == _outPort))
   {
      _outAddr = _inAddr;
      _sipperOutSocket = _sipperInSocket;
   }
   else
   {
      if((_sipperOutSocket = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
      {
         logger.logMsg(ERROR_FLAG, 0, "Socket creation error. [%s]\n",
                       strerror(SipperProxyPortable::getErrorCode()));
         exit(1);
      }

      u_int flagOn = 1;
#ifndef __UNIX__
      setsockopt(_sipperOutSocket, SOL_SOCKET, SO_REUSEADDR, 
                 (const char *)&flagOn, sizeof(flagOn));
#else
      setsockopt(_sipperOutSocket, SOL_SOCKET, SO_REUSEADDR, 
                 &flagOn, sizeof(flagOn));
#endif

      _outAddr = new sockaddr_in();
      memset(_outAddr, 0, sizeof(sockaddr_in));

      _outAddr->sin_family = AF_INET;
      _outAddr->sin_addr.s_addr = inet_addr(_outStrAddr.c_str());
      _outAddr->sin_port = htons(_outPort);

      if(bind(_sipperOutSocket, (sockaddr *)_outAddr, sizeof(sockaddr_in)) < 0)
      {
         logger.logMsg(ERROR_FLAG, 0,
                       "Error binding port[%d]. [%s]\n", _outPort,
                       strerror(SipperProxyPortable::getErrorCode()));
         exit(1);
      }

      SipperProxyPortable::setNonBlocking(_sipperOutSocket);
   }

   int numOfSipper = config.getConfig("Global", "NumSipperDomain", "0");

   if(numOfSipper == 0)
   {
      logger.logMsg(ERROR_FLAG, 0,
                    "Num of SipperDomain is invalid [%d].\n", _numOfSipperDomain);
      exit(1);
   }

   sipperDomains = new SipperDomain[numOfSipper];

   for(unsigned int idx = 0; idx < numOfSipper; idx++)
   {
      char domainname[100];
      sprintf(domainname, "SipperDomain%d", idx + 1);

      std::string ipstr = config.getConfig(domainname, "Ip", "");

      if(ipstr == "")
      {
         logger.logMsg(ERROR_FLAG, 0,
                       "Invalid IP for Domain[%s].\n", domainname);
         exit(1);
      }

      sipperDomains[idx].ip   = inet_addr(ipstr.c_str());
      sipperDomains[idx].name = config.getConfig(domainname, "Name", sipperDomains[idx].ip);
      sipperDomains[idx].port = config.getConfig(domainname, "Port", "5060");
   }
}

SipperProxy::~SipperProxy()
{
   if(_inAddr == _outAddr)
   {
      if(_inAddr != NULL) delete _inAddr;

      if(_sipperInSocket != -1) 
         SipperProxyPortable::disconnectSocket(_sipperInSocket);
   }
   else
   {
      if(_inAddr != NULL) delete _inAddr;
      if(_outAddr != NULL) delete _outAddr;

      if(_sipperInSocket != -1) 
         SipperProxyPortable::disconnectSocket(_sipperInSocket);
      if(_sipperOutSocket != -1) 
         SipperProxyPortable::disconnectSocket(_sipperOutSocket);
   }

   if(sipperDomains != NULL)
   {
      delete []sipperDomains;
   }
}

void SipperProxy::start()
{
   fd_set read_fds;

   int maxSock = _sipperInSocket;

   if(_sipperOutSocket > maxSock) maxSock = _sipperOutSocket;

   SipperProxyMsg msg;

   while(true)
   {
      FD_ZERO(&read_fds);
      FD_SET(_sipperInSocket, &read_fds);
      FD_SET(_sipperOutSocket, &read_fds);

      struct timeval time_out;
      time_out.tv_sec = 5;
      time_out.tv_usec = 0;

      _dnsCache.checkCache();

      if(select(maxSock + 1, &read_fds, NULL, NULL, &time_out) == -1)
      {
         std::string errMsg = SipperProxyPortable::errorString();
         logger.logMsg(ERROR_FLAG, 0, "Error getting socket status. [%s]\n",
                       errMsg.c_str());
         continue;
      }

      memset(&msg.recvSource, 0, sizeof(sockaddr_in));
#ifdef __UNIX__
      socklen_t clilen = sizeof(sockaddr_in);
#else
      int clilen = sizeof(sockaddr_in);
#endif

      if(FD_ISSET(_sipperInSocket, &read_fds))
      {
         if((msg.bufferLen = recvfrom(_sipperInSocket, msg.buffer, 
                                      MAX_PROXY_MSG_LEN, 0, 
                                      (struct sockaddr *)&msg.recvSource, 
                                      &clilen)) <= 0)
         {
            continue;
         }

         msg.recvSocket = _sipperInSocket;
         msg.sendSocket = _sipperOutSocket;
      }
      else if(FD_ISSET(_sipperOutSocket, &read_fds))
      {
         if((msg.bufferLen = recvfrom(_sipperOutSocket, msg.buffer, 
                                      MAX_PROXY_MSG_LEN, 0, 
                                      (struct sockaddr *)&msg.recvSource, 
                                      &clilen)) <= 0)
         {
            continue;
         }

         msg.recvSocket = _sipperOutSocket;
         msg.sendSocket = _sipperInSocket;
      }
      else
      {
         continue;
      }

      msg.processMessage(this);
   }
}

void SipperProxyMsg::processMessage(SipperProxy *context)
{
   _context = context;

   if(bufferLen < 0 || bufferLen > MAX_PROXY_MSG_LEN)
   {
      logger.logMsg(ERROR_FLAG, 0, "Invalid MessageLen [%d]\n", bufferLen);
      return;
   }

   buffer[bufferLen] = '\0';

   logger.logMsg(TRACE_FLAG, 0, "ReceivedMessage From[%s:%d]\n---\n[%s]\n---",
                 inet_ntoa(recvSource.sin_addr), ntohs(recvSource.sin_port),
                 buffer);

   if(strncmp(buffer, "SIP/2.0", 7) == 0)
   {
      _processResponse();
   }
   else
   {
      char *end = strstr(buffer, "\r\n");
      if(end == NULL)
      {
         logger.logMsg(ERROR_FLAG, 0, "Invalid Message [%s]\n", buffer);
         return;
      }
      
      end -= 7;

      if((end > buffer) && (strncmp(end, "SIP/2.0", 7) == 0))
      {
         hdrStart = end + 9;
         _processRequest();
      }
      else
      {
         logger.logMsg(ERROR_FLAG, 0, "Invalid Message [%s]\n", buffer);
         return;
      }
   }
}

void SipperProxyMsg::_processResponse()
{
   if(_removeFirstVia() == -1)
   {
      logger.logMsg(ERROR_FLAG, 0, "Failed to remove VIA [%s]\n", buffer);
      return;
   }

   if(_setTargetFromFirstVia() == -1)
   {
      logger.logMsg(ERROR_FLAG, 0, "Error getting Targer from VIA [%s]\n", 
                    buffer);
      return;
   }

   logger.logMsg(TRACE_FLAG, 0, "SendingResponse To[%s:%d] Len[%d]\n---\n[%s]\n---",
                 inet_ntoa(sendTarget.sin_addr), ntohs(sendTarget.sin_port),
                 bufferLen, buffer);

   sendto(sendSocket, buffer, bufferLen, 0, (struct sockaddr *)&sendTarget,
          sizeof(sockaddr_in));
}

int SipperProxyMsg::_getFirstVia(char *&viaStart, char *&viaValStart)
{
   char *fullForm = strstr(hdrStart - 2, "\r\nVia:");
   char *shortForm = NULL;

   if(fullForm != NULL)
   {
      char locTmp = *fullForm;
      *fullForm = '\0';
      shortForm = strstr(hdrStart - 2, "\r\nv:");
      *fullForm = locTmp;
   }
   else
   {
      shortForm = strstr(hdrStart - 2, "\r\nv:");
   }

   char *viaToUse = fullForm;

   if(viaToUse == NULL) 
   {
      viaToUse = shortForm;
   }
   else 
   {
      if((shortForm != NULL) && (shortForm < fullForm))
      {
         viaToUse = shortForm;
      }
   }

   if(viaToUse == NULL)
   {
      logger.logMsg(ERROR_FLAG, 0, "No Via found. \n");
      return -1;
   }

   if(viaToUse == fullForm) 
   {
      viaStart = fullForm + 2;
      viaValStart = fullForm + 6;
   }
   else
   {
      viaStart = shortForm + 2;
      viaValStart = shortForm + 4;
   }

   return 0;
}

void SipperProxyMsg::_removeData(char *from, char *to)
{
   if(to < from) return;

   memmove(from, to, (buffer + bufferLen) - to);
   bufferLen -= (to - from);
}

int SipperProxyMsg::_removeFirstVia()
{
   char *viaStart = NULL;
   char *viaValStart = NULL;

   if(_getFirstVia(viaStart, viaValStart) == -1)
   {
      return -1;
   }

   char *viaEnd = strstr(viaValStart, "\r\n");

   if(viaEnd == NULL) 
   {
      logger.logMsg(ERROR_FLAG, 0, "No Via End found.\n");
      return -1;
   }

   char tmpData = *viaEnd;
   *viaEnd = '\0';
   char *viaValEnd = strstr(viaValStart, ",");
   *viaEnd = tmpData;

   if((viaValEnd == NULL) || (viaEnd < viaValEnd))
   {
      //RemoveFullHeader
      _removeData(viaStart, viaEnd + 2);
   }
   else
   {
      //Comma separated value. Remove till ,.
      _removeData(viaValStart, viaValEnd + 1);
   }
}

int SipperProxyMsg::_setTargetFromFirstVia()
{
   memset(&sendTarget, 0, sizeof(sendTarget));
   sendTarget.sin_family = AF_INET;

   char *viaStart = NULL;
   char *viaValStart = NULL;

   if(_getFirstVia(viaStart, viaValStart) == -1)
   {
      return -1;
   }

   while(*viaValStart == ' ') viaValStart++;

   while(*viaValStart != ' ' && *viaValStart != '\0' &&
         *viaValStart != ',' && *viaValStart != '\r') viaValStart++;

   if(*viaValStart != ' ') return -1;

   while(*viaValStart == ' ') viaValStart++;

   char *hostStart = viaValStart;
   char *portStart = viaValStart;

   while(*portStart != ':' && *portStart != '\r' && *portStart != ',' && *portStart != '\0')
   {
      portStart++;
   }

   unsigned short port = 5060;

   if(*portStart == ':')
   {
      port = atoi(portStart + 1);
   }

   std::string hostname(hostStart, portStart - hostStart);

   sendTarget.sin_port = htons(port);
   sendTarget.sin_addr.s_addr = _context->getIp(hostname);

   if(sendTarget.sin_addr.s_addr == -1)
   {
      return -1;
   }

   return 0;
}

void SipperProxyMsg::_processRequest()
{
   _processViaRport();
   _addViaHeader();

   if(_isRegisterRequest())
   {
      _addPathHeader();
   }
   else
   {
      _addRecordRouteHeader();
   }

   if(_isRoutePresent())
   {
      if(_isReqURIContainsProxyDomain())
      {
         //Message from StrictRouter.
         _removeReqURI();
         _moveLastRouteToReqURI();
      }
      else
      {
         _removeFirstRouteIfProxyDomain();
      }

      if(_setTargetFromFirstRoute() == -2)
      {
         _setTargetFromReqURI();
      }
   }
   else
   {
      if(_isReqURIIsProxyDomain())
      {
         //Choose one from the SipperDomain
         _setTargetFromSipperDomain();
      }
      else
      {
         _setTargetFromReqURI();
      }
   }

   logger.logMsg(TRACE_FLAG, 0, "SendingRequest To[%s:%d] Len[%d]\n---\n[%s]\n---",
                 inet_ntoa(sendTarget.sin_addr), ntohs(sendTarget.sin_port),
                 bufferLen, buffer);

   sendto(sendSocket, buffer, bufferLen, 0, (struct sockaddr *)&sendTarget,
          sizeof(sockaddr_in));
}
