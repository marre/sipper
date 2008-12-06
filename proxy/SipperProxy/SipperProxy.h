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

      void addEntry(const std::string &name, in_addr_t ip, bool removable = false);
};

class SipperDomain
{
   public:

      std::string name;
      in_addr_t ip;
      unsigned short port;

      std::string hostpart;
};

class SipperProxy
{
   private:

      int _sipperOutSocket;
      int _sipperInSocket;

      struct sockaddr_in *_inAddr;
      struct sockaddr_in *_outAddr;

      std::string _inStrPort;
      unsigned short _inPort;
      unsigned short _outPort;

      std::string _inStrAddr;
      std::string _outStrAddr;

      DnsCache _dnsCache;

   public:

      unsigned int _numOfSipperDomain;
      unsigned int _toSendIndex;

      SipperDomain *sipperDomains;

      std::string pxyStrIp;     //68.178.254.124
      std::string pxyStrDomain; //sip.agnity.com
      std::string pxyStrPort;   //5060

      std::string pxyRouteHdr;
      std::string pxyPathHdr;
      std::string pxyRecordRouteHdr;
      std::string pxyUriHost;   //sip.agnity.com:5060
      std::string pxyUriIPHost; //68.178.254.124:5060
      std::string pxyViaHdr;

      bool incPathHdr;
      bool incRecordRouteHdr;

   public:

      SipperProxy();
      ~SipperProxy();
      void start();

      SipperDomain * getSipperDomain();

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

      char branch[301];

      struct sockaddr_in recvSource;
      int recvSocket;

      struct sockaddr_in sendTarget;
      int sendSocket;

      char *hdrStart;

      SipperProxy *_context;

      char *_routeStart;

   public:

      void processMessage(SipperProxy *context);

   private:

      void _processResponse();
      void _processRequest();

      int _removeFirstVia();
      int _setTargetFromFirstVia();

      int _getFirstVia(char *&viaStart, char *&viaValStart);

      void _removeData(char *from, char *to);
      void _addToBuffer(char *startPos, const char *insData, int len);
      void _replaceData(char *from, char *to, const char *insData, int len);

      int _processViaRport();
      void _addViaHeader();
      void _addPathHeader();
      void _addRecordRouteHeader();

      int _getFirstRoute(char *&routeStart, char *&routeValStart);
      int _getLastRoute(char *&routeStart, char *&routeEnd, bool &singleHdr,
                        char *&routeValStart, char *&routeValEnd);

      bool _isRegisterRequest();
      bool _isRoutePresent();
      bool _isReqURIContainsProxyDomain();
      void _moveLastRouteToReqURI();
      void _removeFirstRouteIfProxyDomain();
      int _setTargetFromFirstRoute();
      int _setTargetFromReqURI();
      int _setTargetFromSipperDomain();
};

#endif
