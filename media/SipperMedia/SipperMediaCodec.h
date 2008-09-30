#ifndef __SIPPER_MEDIA_CODEC_H__
#define __SIPPER_MEDIA_CODEC_H__

#pragma warning(disable: 4786)
#include <string>
#include <list>

#ifndef __UNIX__
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

#include "SipperMediaFileLoader.h"
#include "SipperMediaProtocolHeader.h"

class SipperMedia;


class SipperMediaCodec
{
public:

   int recvPayloadNum;
   int sendPayloadNum;

public:

   SipperMedia *_media;

   virtual ~SipperMediaCodec()
   {
   }
   virtual void handleTimer(struct timeval &currtime) = 0;
   virtual void checkActivity(struct timeval &currtime) = 0;
   virtual void processReceivedRTPPacket(struct timeval &currtime, const char *payload, unsigned int payloadlen) = 0;
   virtual void processReceviedPacket(struct timeval &currtime, const char *data, unsigned int datalen)
   {
   }
};

typedef std::list<std::string> CommandList;
typedef CommandList::iterator CommandListIt;

class SipperMediaG711Codec : public SipperMediaCodec
{
private:

   struct timeval _lastrecvTime;
   int _lastTimestamp;

   SipperMediaRefObjHolder audioContentHolder;

   int _offset;

   static unsigned char a2u[128];
   static unsigned char u2a[128];

   /* A-law to u-law conversion */
   static int alaw2ulaw (int   aval)
   {
      aval &= 0xff;
      return ((aval & 0x80) ? (0xFF ^ a2u[aval ^ 0xD5]) :
            (0x7F ^ a2u[aval ^ 0x55]));
   }

   static int ulaw2alaw (int   uval)
   {
      uval &= 0xff;
      return ((uval & 0x80) ? (0xD5 ^ (u2a[0xFF ^ uval] - 1)) :
         (0x55 ^ (u2a[0x7F ^ uval] - 1)));
   }

public:

   enum CodecType
   {
      G711U = 1,
      G711A
   };

private:

   CodecType _type;
   int _outfilelen;
   FILE *_outfile;

   CommandList _commandList;
   struct timeval _wakeupTime;
   int _commandLen;
   bool _repeatFlag;

   void _loadCommandLine(const std::string &sendFile);
   void _startCommandProcessing();
public:

   SipperMediaG711Codec(CodecType type, const std::string &sendFile, const std::string &recvFile);
   ~SipperMediaG711Codec();

   void handleTimer(struct timeval &currtime);
   void checkActivity(struct timeval &currtime);
   void processReceivedRTPPacket(struct timeval &currtime, const char *payload, unsigned int payloadlen);
};

class SipperMediaDTMFCodec : public SipperMediaCodec
{
private:

   enum SipperDTMFSendState
   {
      STATE_NONE,
      STATE_SLEEPING,
      STATE_SENDING
   };

   SipperDTMFSendState _sendState;
   CommandList _commandList;
   int _listLen;
   struct timeval _wakeupTime;

   SipperMediaRTPHeader _dtmfRTPHeader;
   SipperMediaDTMFPacket _dtmfPacket;
   int _dtmfPacketCount;

public:

   SipperMediaDTMFCodec(const std::string &sendFile, const std::string &recvFile);
   void sendDtmf(const std::string & command, bool checkFlag = 1);
   void handleTimer(struct timeval &currtime);
   void checkActivity(struct timeval &currtime);
   void processReceivedRTPPacket(struct timeval &currtime, const char *payload, unsigned int payloadlen);
};

#endif
