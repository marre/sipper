#include "SipperMediaLogger.h"
LOG("SipperMediaCodec");
#include "SipperMediaCodec.h"
#include "SipperMedia.h"
#include "SipperMediaTokenizer.h"
#include "SipperMediaPortable.h"
#include "vector"

unsigned char SipperMediaG711Codec::a2u[128] = {
   1,   3,   5,   7,   9,   11,   13,   15,
   16,   17,   18,   19,   20,   21,   22,   23,
   24,   25,   26,   27,   28,   29,   30,   31,
   32,   32,   33,   33,   34,   34,   35,   35,
   36,   37,   38,   39,   40,   41,   42,   43,
   44,   45,   46,   47,   48,   48,   49,   49,
   50,   51,   52,   53,   54,   55,   56,   57,
   58,   59,   60,   61,   62,   63,   64,   64,
   65,   66,   67,   68,   69,   70,   71,   72,
   73,   74,   75,   76,   77,   78,   79,   80,
   80,   81,   82,   83,   84,   85,   86,   87,
   88,   89,   90,   91,   92,   93,   94,   95,
   96,   97,   98,   99,   100,   101,   102,   103,
   104,   105,   106,   107,   108,   109,   110,   111,
   112,   113,   114,   115,   116,   117,   118,   119,
   120,   121,   122,   123,   124,   125,   126,   127};

unsigned char SipperMediaG711Codec::u2a[128] = {
   1,   1,   2,   2,   3,   3,   4,   4,
   5,   5,   6,   6,   7,   7,   8,   8,
   9,   10,   11,   12,   13,   14,   15,   16,
   17,   18,   19,   20,   21,   22,   23,   24,
   25,   27,   29,   31,   33,   34,   35,   36,
   37,   38,   39,   40,   41,   42,   43,   44,
   46,   48,   49,   50,   51,   52,   53,   54,
   55,   56,   57,   58,   59,   60,   61,   62,
   64,   65,   66,   67,   68,   69,   70,   71,
   72,   73,   74,   75,   76,   77,   78,   79,
   80,   82,   83,   84,   85,   86,   87,   88,
   89,   90,   91,   92,   93,   94,   95,   96,
   97,   98,   99,   100,   101,   102,   103,   104,
   105,   106,   107,   108,   109,   110,   111,   112,
   113,   114,   115,   116,   117,   118,   119,   120,
   121,   122,   123,   124,   125,   126,   127,   128};

SipperMediaG711Codec::SipperMediaG711Codec(CodecType type, const std::string &sendFile, const std::string &recvFile)
{
   _lastrecvTime.tv_sec = 0;
   _lastrecvTime.tv_usec = 0;
   _lastTimestamp = 0;

   _type = SipperMediaG711Codec::G711U;

   if(type == SipperMediaG711Codec::G711A)
   {
      _type = SipperMediaG711Codec::G711A;
   }

   logger.logMsg(TRACE_FLAG, 0, "CodecType[%d] SendFile[%s] RecvFile[%s]\n", 
                 _type, sendFile.c_str(), recvFile.c_str());

   _outfile = NULL;
   _outfilelen = 0;

   if(recvFile.length() > 0)
   {
      _outfile = fopen(recvFile.c_str(), "w+");

      if(_outfile == NULL)
      {
         logger.logMsg(ERROR_FLAG, 0, "Error opening File[%s]\n", recvFile.c_str());
      }
   }

   if(_outfile)
   {
      fprintf(_outfile, ".snd");

      int data = 24 + 12;
      data = htonl(data);
      fwrite(&data, sizeof(int), 1, _outfile);

      data = 0xFFFFFFFF;
      fwrite(&data, sizeof(int), 1, _outfile);

      data = 1;
      data = htonl(data);
      fwrite(&data, sizeof(int), 1, _outfile);

      data = 8000;
      data = htonl(data);
      fwrite(&data, sizeof(int), 1, _outfile);

      data = 1;
      data = htonl(data);
      fwrite(&data, sizeof(int), 1, _outfile);

      fprintf(_outfile, "SIPPERMEDIA");
      data = 0;
      fwrite(&data, 1, 1, _outfile);
   }

   _loadCommandLine(sendFile);

   audioContentHolder.setObj(SipperMediaFileLoader::getInstance().loadFile(""));
   _offset = 0;
   _wakeupTime.tv_sec = 0;
   _wakeupTime.tv_usec = 0;
}

void SipperMediaG711Codec::_loadCommandLine(const std::string &command)
{
   std::string delimiter = ",";
    std::insert_iterator<CommandList> inserter(_commandList, _commandList.end());

    SipperMediaTokenizer(command, delimiter, inserter);
   _commandLen = _commandList.size();
}

typedef std::vector<std::string> CommandVec;
typedef CommandVec::iterator CommandVecIt;

void SipperMediaG711Codec::_startCommandProcessing()
{
   std::string currcommand;

   if(_commandList.size() != 0)
   {
      currcommand = _commandList.front();
      _commandList.pop_front();

      if(currcommand.size() == 0)
      {
         _startCommandProcessing();
         return;
      }
   }
   else
   {
      currcommand = "PLAY_REPEAT 0 0";
   }

   SipperMediaPortable::trim(currcommand);

   logger.logMsg(TRACE_FLAG, 0, "Processing command [%s].\n", currcommand.c_str());

   CommandVec playcommand;
   std::string delimiter = " ";
   std::insert_iterator<CommandVec> inserter(playcommand, playcommand.begin());

   SipperMediaTokenizer(currcommand, delimiter, inserter);

   if(playcommand.size() == 0)
   {
      _startCommandProcessing();
      return;
   }

   SipperMediaPortable::toUpper(playcommand[0]);

   if(playcommand[0] == "SLEEP")
   {
      std::string duration("0");
      if(playcommand.size() > 1)
      {
         duration = playcommand[1];
      }
      playcommand.clear();

      playcommand.push_back("PLAY_REPEAT");
      playcommand.push_back("0");
      playcommand.push_back(duration);
   }
   else if((playcommand[0] != "PLAY") && (playcommand[0] != "PLAY_REPEAT"))
   {
      std::string loccommand = playcommand[0];
      playcommand.clear();
      playcommand.push_back("PLAY_REPEAT");
      playcommand.push_back(loccommand);
      playcommand.push_back("0");
   }

   while(playcommand.size() < 3)
   {
      playcommand.push_back("0");
   }

   if(playcommand[0] == "PLAY")
   {
      _repeatFlag = false;
   }
   else
   {
      _repeatFlag = true;
   }

   audioContentHolder.setObj(SipperMediaFileLoader::getInstance().loadFile(playcommand[1]));
   _offset = 0;

   int duration = atoi(playcommand[2].c_str());
   
   if(duration == 0)
   {
      _wakeupTime.tv_sec = 0x7FFFFFFF;
      _wakeupTime.tv_usec = 0;
   }
   else
   {
      SipperMediaPortable::getTimeOfDay(&_wakeupTime);
      _wakeupTime.tv_sec += duration;
   }
}

SipperMediaG711Codec::~SipperMediaG711Codec()
{
   if(_outfile != NULL)
   {
      fseek(_outfile, 8, SEEK_SET);
      int data = htonl(_outfilelen);
      fwrite(&data, sizeof(int), 1, _outfile);
      fclose(_outfile);

      _outfile = NULL;
   }
}

void SipperMediaG711Codec::handleTimer(struct timeval &currtime)
{
   SipperRTPMedia *media = static_cast<SipperRTPMedia *>(_media);
   SipperMediaRTPHeader header = media->lastSentHeader;
   header.setVersion(2);
   header.setPadding(0);
   header.setExtension(0);
   header.setCSRCCount(0);
   header.setMarker(0);
   header.setPayloadNum(sendPayloadNum);
   header.setSequence(header.getSequence() + 1);

   if(_lastTimestamp == 0)
   {
      _lastTimestamp = header.getTimeStamp();
   }
   _lastTimestamp += 160;
   header.setTimeStamp(_lastTimestamp);

   SipperMediaFileContent *g711Content = dynamic_cast<SipperMediaFileContent *>(audioContentHolder.getObj());

   char *dataptr = g711Content->data + _offset;

   bool rollover = false;
   bool deletecontent = false;
   if(_offset + 160 >= g711Content->len)
   {
      char *tmp = new char[160];
      memset(tmp, 0xFF, 160);
      memcpy(tmp, dataptr, g711Content->len - _offset);
      dataptr = tmp;
      deletecontent = true;
      rollover = true;
      _offset = 0;
   }
   else
   {
      _offset += 160;
   }

   if(_type == SipperMediaG711Codec::G711A)
   {
      if(!deletecontent)
      {
         char *tmp = new char[160];
         memcpy(tmp, dataptr, 160);
         dataptr = tmp;
         deletecontent = true;
      }

      for(int idx = 0; idx < 160; idx++)
      {
         dataptr[idx] = ulaw2alaw(dataptr[idx]);
      }
   }

   media->sendRTPPacket(header, dataptr, 160);

   if(deletecontent)
   {
      delete []dataptr;
   }

   if(rollover && (!_repeatFlag))
   {
       logger.logMsg(TRACE_FLAG, 0, "End of play.\n");
      _startCommandProcessing();
      return;
   }

   if(SipperMediaPortable::isGreater(&currtime, &_wakeupTime))
   {
       logger.logMsg(TRACE_FLAG, 0, "End of duration[%d-%d] [%d-%d].\n",
                      currtime.tv_sec, currtime.tv_usec, _wakeupTime.tv_sec,
                      _wakeupTime.tv_usec);
      _startCommandProcessing();
      return;
   }
}

void SipperMediaG711Codec::checkActivity(struct timeval &currtime)
{
   if((_lastrecvTime.tv_sec == 0) && (_lastrecvTime.tv_usec == 0))
   {
      return;
   }

   struct timeval tolerance = _lastrecvTime;
   tolerance.tv_sec += 5;

   if(SipperMediaPortable::isGreater(&currtime, &tolerance))
   {
      char evt[200];
      sprintf(evt, "CODEC=%d;EVENT=AUDIOSTOPPED", this->recvPayloadNum);
      _media->sendEvent(evt);
      _lastrecvTime.tv_sec = 0;
      _lastrecvTime.tv_usec = 0;
   }
}

void SipperMediaG711Codec::processReceivedRTPPacket(struct timeval &currtime, const char *payload, unsigned int payloadlen)
{
   if((_lastrecvTime.tv_sec == 0) && (_lastrecvTime.tv_usec == 0))
   {
      char evt[200];
      sprintf(evt, "CODEC=%d;EVENT=AUDIOSTARTED", this->recvPayloadNum);
      _media->sendEvent(evt);
   }

   _lastrecvTime = currtime;

   if(_outfile == NULL)
   {
      return;
   }

   char *newpayload = const_cast<char *>(payload);

   if(_type == SipperMediaG711Codec::G711A)
   {
      newpayload = new char[payloadlen];

      for(unsigned int idx = 0; idx < payloadlen; idx++)
      {
         newpayload[idx] = alaw2ulaw(payload[idx]);
      }
   }

   fwrite(newpayload, payloadlen, 1, _outfile);
   _outfilelen += payloadlen;

   if(_type == G711A)
   {
      delete []newpayload;
   }
}

SipperMediaDTMFCodec::SipperMediaDTMFCodec(const std::string &sendFile, const std::string &recvFile)
{
   _sendState = STATE_NONE;
   _listLen = 0;

   if(sendFile.length() > 0)
   {
      FILE *fp = fopen(sendFile.c_str(), "r");

      if(fp != NULL)
      {
         char data[201]; data[200] = '\0';
         while(fgets(data, 200, fp) != NULL)
         {
            sendDtmf(data);
         }

         fclose(fp);
      }
   }
}

void SipperMediaDTMFCodec::sendDtmf(const std::string & command, bool checkFlag)
{
   std::string delimiter = ",";
    std::insert_iterator<CommandList> inserter(_commandList, _commandList.end());

    SipperMediaTokenizer(command, delimiter, inserter);
   _listLen = _commandList.size();
}

void SipperMediaDTMFCodec::handleTimer(struct timeval &currtime)
{
   switch(_sendState)
   {
   case STATE_NONE:
      {
         if(_listLen == 0) return;
         std::string currcommand = _commandList.front();
         _commandList.pop_front();
         _listLen--;

		 SipperMediaPortable::toUpper(currcommand);
		 SipperMediaPortable::trim(currcommand);

         char *command = (char *)currcommand.c_str();
         char dummy[100];
         if(currcommand.length() > 50)
         {
            command[50] = '\0';
         }

         if(*command == 'S')
         {
            logger.logMsg(TRACE_FLAG, 0, 
                          "Processing DTMF SLEEP command [%s].\n", currcommand.c_str());
            int sleepDuration;
            sscanf(command, "%s %d", dummy, &sleepDuration);
            _wakeupTime = currtime;
            _wakeupTime.tv_sec += sleepDuration;
            _sendState = STATE_SLEEPING;
         }
         else
         {
            int toSend = atoi(command);
            logger.logMsg(TRACE_FLAG, 0, 
                          "Processing DTMF Digit[%d] command[%s].\n", 
                          toSend, currcommand.c_str());
            SipperRTPMedia *media = static_cast<SipperRTPMedia *>(_media);
            _dtmfRTPHeader = media->lastSentHeader;
            _dtmfRTPHeader.setVersion(2);
            _dtmfRTPHeader.setPadding(0);
            _dtmfRTPHeader.setExtension(0);   
            _dtmfRTPHeader.setCSRCCount(0);
            _dtmfRTPHeader.setMarker(1);
            _dtmfRTPHeader.setPayloadNum(sendPayloadNum);
            _dtmfRTPHeader.setSequence(media->lastSentHeader.getSequence() + 1);
            _dtmfPacket.setEndBit(0);
            _dtmfPacket.setReserve(0);
            _dtmfPacket.setEvent(toSend);
            _dtmfPacket.setVolume(12);
            _dtmfPacket.setDuration(0);
            int rtpPayload = htonl(_dtmfPacket.first);
            media->sendRTPPacket(_dtmfRTPHeader, (char *)&rtpPayload, 4);
            _dtmfRTPHeader.setMarker(0);
            _dtmfPacketCount = 1;
            _sendState = STATE_SENDING;
         }
      };
      break;
   case STATE_SLEEPING:
      {
         if(SipperMediaPortable::isGreater(&currtime, &_wakeupTime))
         {
            _sendState = STATE_NONE;
         }
      };
      break;
   case STATE_SENDING:
      {
         SipperRTPMedia *media = static_cast<SipperRTPMedia *>(_media);
         _dtmfRTPHeader.setSequence(media->lastSentHeader.getSequence() + 1);

         if(_dtmfPacketCount < 7)
         {
            _dtmfPacket.setDuration(_dtmfPacket.getDuration() + 160);
         }
         else if(_dtmfPacketCount == 7)
         {
            _dtmfPacket.setDuration(_dtmfPacket.getDuration() + 160);
            _dtmfPacket.setEndBit(1);
         }
         else if(_dtmfPacketCount == 10)
         {
            _sendState = STATE_NONE;
         }

         int rtpPayload = htonl(_dtmfPacket.first);
         media->sendRTPPacket(_dtmfRTPHeader, (char *)&rtpPayload, 4);
         _dtmfPacketCount++;
      };
      break;
   }
}

void SipperMediaDTMFCodec::checkActivity(struct timeval &currtime)
{
   return;
}

void SipperMediaDTMFCodec::processReceivedRTPPacket(struct timeval &currtime, const char *payload, unsigned int payloadlen)
{
   if(payloadlen == 4)
   {
      SipperRTPMedia *media = static_cast<SipperRTPMedia *>(_media);
      SipperMediaRTPHeader header = media->lastRecvHeader;
      if(media->lastDtmfTimestamp == header.getTimeStamp())
      {
         return;
      }

      media->lastDtmfTimestamp = header.getTimeStamp();

      char evt[200];
      sprintf(evt, "CODEC=%d;EVENT=DTMFRECEIVED;DTMF=%d", this->recvPayloadNum, payload[0]);
      _media->sendEvent(evt);
   }
}

