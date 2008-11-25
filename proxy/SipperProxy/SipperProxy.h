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

#include <map>
#include <string>

struct DnsEntry
{
   public:

      time_t entryTime;

      int entryCount;
      in_addr_t addr[5];

   public:

      DnsEntry()
      {
         entryTime = 0;
         entryCount = 0;
         for(unsigned int idx = 0; idx < 5; idx++)
         {
            addr[idx] = -1;
         }
      }
};

typedef std::map<std::string, DnsEntry> DnsMap;
typedef DnsMap::const_iterator DnsMapCIt;
typedef DnsMap::iterator DnsMapIt;

class DnsCache
{
   private:

      DnsMap _entries;
      time_t _lastCheckTime;

   public:

      DnsCache();

      in_addr_t getIp(const std::string &hostname);
      void checkCache();
};

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

      DnsCache _dnsCache;

   public:

      SipperProxy();
      ~SipperProxy();
      void start();

      in_addr_t getIp(const std::string &hostname)
      {
         return _dnsCache.getIp(hostname);
      }
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

      int _getFirstVia(char *&viaStart, char *&viaValStart);
      void _removeData(char *from, char *to);


};

#endif
