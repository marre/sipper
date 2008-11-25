#ifndef __PROXY_MSG_H__
#define __PROXY_MSG_H__

#ifndef __UNIX__
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

class SipperProxy
{
   private:

      int _sipperOutSocket;
      int _sipperInSocket;

      struct sockaddr_in *_inAddr;
      struct sockaddr_in *_outAddr;

      unsigned short _inPort;
      unsigned short _outPort;

      std::string _inStrAddr;
      std::string _outStrAddr;

      std::string _inDomain;   //Hostname or IP.
      std::string _outDomain;

   public:

      SipperProxy();
      ~SipperProxy();
      void start();
};

#define MAX_PROXY_MSG_LEN 0xFFFF

class SipperProxyMsg
{
   public:

      char buffer[MAX_PROXY_MSG_LEN + 1];
      int  bufferLen;

      struct sockaddr_in recvSource;
      int recvSocket;

      struct sockaddr_in sendTarget;
      int sendSocket;

      char *hdrStart;

      SipperProxy *_context;

   public:

      void processMessage(SipperProxy *context);

   private:

      void _processResponse();
      void _processRequest();

      int _removeFirstVia();
      int _setTargetFromFirstVia();
};

#endif
