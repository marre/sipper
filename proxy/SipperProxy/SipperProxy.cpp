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

void DnsCache::addEntry(const std::string &name, in_addr_t ip, bool removable)
{
   DnsEntry entry;
   entry.addr[0] = ip;
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
   _inAddr(NULL),
   _outAddr(NULL),
   _inPort(0),
   _outPort(0),
   numOfSipperDomain(0),
   toSendIndex(0),
   sipperDomains(NULL)
{
   SipperProxyConfig &config = SipperProxyConfig::getInstance();

   _inStrAddr = config.getConfig("Global", "InAddr", "127.0.0.1");
   //_outStrAddr = config.getConfig("Global", "OutAddr", "");

   if(_outStrAddr == "")
   {
      _outStrAddr = _inStrAddr;
   }

   _inStrPort = config.getConfig("Global", "InPort", "5700");
   _inPort = (unsigned short) atoi(_inStrPort.c_str());
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

   int numOfSipper = atoi(config.getConfig("Global", "NumSipperDomain", "0").c_str());

   if(numOfSipper == 0)
   {
      logger.logMsg(ERROR_FLAG, 0,
                    "Num of SipperDomain is invalid [%d].\n", numOfSipperDomain);
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
                       "Empty IP for Domain[%s].\n", domainname);
         exit(1);
      }

      sipperDomains[idx].ip   = inet_addr(ipstr.c_str());

      if(sipperDomains[idx].ip == -1)
      {
         logger.logMsg(ERROR_FLAG, 0,
                       "Invalid IP[%s] for Domain[%s].\n", ipstr.c_str(), domainname);
         exit(1);
      }

      sipperDomains[idx].name = config.getConfig(domainname, "Name", ipstr);
      sipperDomains[idx].port = atoi(config.getConfig(domainname, "Port", "5060").c_str());
   }

   pxyStrIp = config.getConfig("ProxyDomain", "Ip", "");

   if(pxyStrIp == "")
   {
      logger.logMsg(ERROR_FLAG, 0, "Empty IP for ProxyDomain.\n");
      exit(1);
   }

   pxyStrDomain = config.getConfig("ProxyDomain", "Name", pxyStrIp);
   pxyStrPort = config.getConfig("ProxyDomain", "Port", "5060");

   pxyViaHdr = "Via: SIP/2.0/UDP " + _inStrAddr + ":" + _inStrPort + ";branch=";
   pxyRouteHdr = "Route: <sip:" + pxyStrDomain + ":" + pxyStrPort + ">\r\n";
   pxyRecordRouteHdr = "Record-Route: <sip:" + pxyStrDomain + ":" + pxyStrPort + ";lr>\r\n";
   pxyPathHdr = "Path: <sip:" + pxyStrDomain + ":" + pxyStrPort + ";lr>\r\n";
   pxyUriHost = pxyStrDomain + ":" + pxyStrPort;
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
      if(_isReqURIContainsProxyDomain())
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

void SipperProxyMsg::_addToBuffer(char *startPos, const char *insData, int len)
{
   memmove(startPos + len, startPos, bufferLen - (startPos - buffer) + 1);
   memcpy(startPos, insData, len);
   bufferLen += len;
}

void SipperProxyMsg::_processViaRport()
{
   branch[0] = '\0';
   char *viaStart = NULL;
   char *viaValStart = NULL;

   if(_getFirstVia(viaStart, viaValStart) == -1)
   {
      return;
   }

   while(*viaValStart != ';' && *viaValStart != '\0' &&
         *viaValStart != ',' && *viaValStart != '\r') viaValStart++;

   char insData[100];
   int insLen= sprintf(insData, ";received=%s", inet_ntoa(recvSource.sin_addr));

   _addToBuffer(viaValStart, insData, insLen);

   if(*viaValStart != ';') return;

   while(*viaValStart == ';')
   {
      if(strncmp(viaValStart, ";rport", 6) == 0)
      {
         viaValStart += 6;
         insLen = sprintf(insData, "=%d", ntohs(recvSource.sin_port));
         _addToBuffer(viaValStart, insData, insLen);
      }
      else if(strncmp(viaValStart, ";branch=", 8) == 0)
      {
         viaValStart += 8;
         char *branchTarget = branch;
         int cnt = 0;
         while(*viaValStart != ';' && *viaValStart != '\0' &&
               *viaValStart != ',' && *viaValStart != '\r' && cnt < 250)
         {
            *branchTarget = *viaValStart;
            branchTarget++;
            viaValStart++;
            cnt++;
         }

         *branchTarget = '\0';
      }

      while(*viaValStart != ';' && *viaValStart != '\0' &&
            *viaValStart != ',' && *viaValStart != '\r') viaValStart++;
   }
}

void SipperProxyMsg::_addViaHeader()
{
   char viaHeader[1000];
   int viaLen = 0;
   if(*branch == '\0')
   {
      struct timeval tv;
      gettimeofday(&tv, NULL);

      viaLen = sprintf(viaHeader, "%sz9hG4bK_%d_%d_%d\r\n", _context->pxyViaHdr.c_str(), 
                       tv.tv_sec, tv.tv_usec, rand());
   }
   else
   {
      viaLen = sprintf(viaHeader, "%s%s_0\r\n", _context->pxyViaHdr.c_str(), branch);
   }

   _addToBuffer(hdrStart, viaHeader, viaLen);
}

void SipperProxyMsg::_addPathHeader()
{
   _addToBuffer(hdrStart, _context->pxyPathHdr.c_str(), 
                _context->pxyPathHdr.length());
}

void SipperProxyMsg::_addRecordRouteHeader()
{
   _addToBuffer(hdrStart, _context->pxyRecordRouteHdr.c_str(), 
                _context->pxyRecordRouteHdr.length());
}

bool SipperProxyMsg::_isRegisterRequest()
{
   if(strncmp(buffer, "REGISTER ", 9) == 0)
   {
      return true;
   }

   return false;
}

bool SipperProxyMsg::_isRoutePresent()
{
   _routeStart = strstr(hdrStart - 2, "\r\nRoute:");
   if(_routeStart != NULL)
   {
      _routeStart += 2;
      return true;
   }

   return false;
}

bool SipperProxyMsg::_isReqURIContainsProxyDomain()
{
   char tmp = *hdrStart;
   *hdrStart = '\0';

   if(strstr(buffer, _context->pxyUriHost.c_str()) != NULL)
   {
      *hdrStart = tmp;
      return true;
   }

   *hdrStart = tmp;
   return false;
}

int SipperProxyMsg::_getFirstRoute(char *&routeStart, char *&routeValStart)
{
   char *routeToUse = strstr(hdrStart - 2, "\r\nRoute:");

   if(routeToUse == NULL)
   {
      logger.logMsg(ERROR_FLAG, 0, "No Route found. \n");
      return -1;
   }

   routeStart = routeToUse + 2;
   routeValStart = routeToUse + 8;

   return 0;
}

int SipperProxyMsg::_getLastRoute(char *&routeStart, char *&routeEnd, bool &singleHdr,
                                  char *&routeValStart, char *&routeValEnd)
{
   char *routeToUse = strstr(hdrStart - 2, "\r\nRoute:");

   while(routeToUse != NULL)
   {
      char *tmpPtr = strstr(routeToUse + 8, "\r\nRoute:");

      if(tmpPtr != NULL) routeToUse = tmpPtr;
   }

   if(routeToUse == NULL)
   {
      logger.logMsg(ERROR_FLAG, 0, "No Route found. \n");
      return -1;
   }

   routeStart = routeToUse + 2;
   routeValStart = routeToUse + 8;

   routeEnd = strstr(routeValStart, "\r\n");

   if(routeEnd == NULL)
   {
      logger.logMsg(ERROR_FLAG, 0, "No Route End found. \n");
      return -1;
   }

   routeValEnd = routeEnd;
   routeEnd += 2;

   singleHdr = true;

   char tmpData = *routeEnd;
   *routeEnd = '\0';
   char *lastComma = rindex(routeValStart, ',');
   *routeEnd = tmpData;

   if(lastComma != NULL)
   {
      singleHdr = false;
      routeValStart = lastComma + 1;
      routeValEnd = routeEnd - 2;
   }

   return 0;
}

void SipperProxyMsg::_removeFirstRouteIfProxyDomain()
{
   char *routeStart = NULL;
   char *routeValStart = NULL;
   if(_getFirstRoute(routeStart, routeValStart) == -1)
   {
      return;
   }

   char *routeEnd = strstr(routeValStart, "\r\n");

   if(routeEnd == NULL)
   {
      return;
   }

   char tmpData = *routeEnd;
   *routeEnd = '\0';
   char *routeValEnd = strstr(routeValStart, ",");
   *routeEnd = tmpData;

   if((routeValEnd == NULL) || (routeEnd < routeValEnd))
   {
      routeValEnd = routeEnd;

      tmpData = *routeValEnd;
      *routeValEnd = '\0';
      if(strstr(routeValStart, _context->pxyUriHost.c_str()) != NULL)
      {
         *routeValEnd = tmpData;;
         _removeData(routeStart, routeEnd + 2);
      }
      else
      {
         *routeValEnd = tmpData;;
      }
   }
   else
   {
      tmpData = *routeValEnd;
      *routeValEnd = '\0';
      if(strstr(routeValStart, _context->pxyUriHost.c_str()) != NULL)
      {
         *routeValEnd = tmpData;;
         _removeData(routeValStart, routeValEnd + 1);
      }
      else
      {
         *routeValEnd = tmpData;;
      }
   }
}

void SipperProxyMsg::_moveLastRouteToReqURI()
{
   char *routeStart = NULL;
   char *routeEnd = NULL;
   char *routeValStart = NULL;
   char *routeValEnd = NULL;
   bool singleHdr = false;

   if(_getLastRoute(routeStart, routeEnd, singleHdr,
                    routeValStart, routeValEnd) == -1)
   {
      return;
   }

   char routeHdr[1000];
   if(singleHdr)
   {
      strncpy(routeHdr, routeValStart, (routeValEnd - routeValStart));
      _removeData(routeStart, routeEnd);
   }
   else
   {
      strncpy(routeHdr, routeValStart, (routeValEnd - routeValStart));
      _removeData(routeValStart -1, routeValEnd);
   }

   char *storedHdr = routeHdr;
   while(*storedHdr == ' ' || *storedHdr == '<') storedHdr++;

   char *storedRouteStart = storedHdr;
   while(*storedHdr != ' ' && *storedHdr != '>' && *storedHdr != '\0') storedHdr++;
   *storedHdr = '\0';

   char tmpData = *hdrStart;
   *hdrStart = '\0';
   char *requriStart = strstr(buffer, " ");
   *hdrStart = tmpData;

   if(requriStart == NULL)
   {
      logger.logMsg(TRACE_FLAG, 0, "Invalid ReqURI.\n");
      return;
   }

   if(hdrStart - 10 > requriStart + 1)
   {
      _removeData(requriStart + 1, hdrStart - 10);
      _addToBuffer(requriStart + 1, storedRouteStart, strlen(storedRouteStart));
   }
}


int SipperProxyMsg::_setTargetFromFirstRoute()
{
}

void SipperProxyMsg::_setTargetFromReqURI()
{
}

void SipperProxyMsg::_setTargetFromSipperDomain()
{
}

