#include "SipperProxyLogger.h"
LOG("ProxyMain");
#include "SipperProxy.h"
#include "SipperProxyConfig.h"
#include "SipperProxyLogMgr.h"

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
   _outPort(0)
{
   SipperProxyConfig &config = SipperProxyConfig::getInstance();

   _inStrAddr = config.getConfig("Global", "InAddr", "127.0.0.1");
   _outStrAddr = config.getConfig("Global", "OutAddr", "127.0.0.1");
   _inPort = (unsigned short) atoi(
                 config.getConfig("Global", "InPort", "5700").c_str());
   _outPort = (unsigned short) atoi(
                 config.getConfig("Global", "OutPort", "5800").c_str());

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

   char *viaStart = NULL;
   char *viaValStart = NULL;

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
      _removedata(viaStart, viaEnd + 2);
   }
   else
   {
      //Comma separated value. Remove till ,.
      _removedata(viaValStart, viaValEnd + 1);
   }
}

int SipperProxyMsg::_setTargetFromFirstVia()
{
   char *viaStart = NULL;
   char *viaValStart = NULL;

   if(_getFirstVia(viaStart, viaValStart) == -1)
   {
      return -1;
   }

   while(*viaValStart == ' ') viaValStart++;

   while(*viaValStart != ' ' && *viaValStart != '\0') viaValStart++;

   while(*viaValStart == ' ') viaValStart++;

   //Get the host and port.
}
