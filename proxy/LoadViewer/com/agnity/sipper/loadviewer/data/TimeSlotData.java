package com.agnity.sipper.loadviewer.data;

import java.util.HashMap;
import java.util.Map.Entry;

public class TimeSlotData
{
    public long                  sec                = 0;
    public long                  numMsgs            = 0;
    public long                  numDroppedMsgs     = 0;
    public long                  numReqs            = 0;
    public long                  numIncomings       = 0;

    public long                  numNewCalls        = 0;
    public long                  numNewTxns         = 0;
    public long                  numReqRetrans      = 0;

    public long                  numCompCalls       = 0;
    public long                  numCompTxns        = 0;

    public long                  numResRetrans      = 0;
    public long                  numProvisionalResp = 0;

    public HashMap<String, Long> incomingMsgMap     = new HashMap<String, Long>(25);
    public HashMap<String, Long> outgoingMsgMap     = new HashMap<String, Long>(25);

    public void reset()
    {
        sec = 0;
        numNewCalls = 0;
        numCompCalls = 0;
        numNewTxns = 0;
        numCompTxns = 0;
        numResRetrans = 0;
        numReqRetrans = 0;
        numDroppedMsgs = 0;
        numMsgs = 0;
        numReqs = 0;
        numIncomings = 0;
        numProvisionalResp = 0;
        incomingMsgMap.clear();
        outgoingMsgMap.clear();
    }

    public void copyTo(TimeSlotData in)
    {
        in.numNewCalls += numNewCalls;
        in.numCompCalls += numCompCalls;
        in.numNewTxns += numNewTxns;
        in.numCompTxns += numCompTxns;
        in.numResRetrans += numResRetrans;
        in.numReqRetrans += numReqRetrans;
        in.numDroppedMsgs += numDroppedMsgs;
        in.numMsgs += numMsgs;
        in.numReqs += numReqs;
        in.numIncomings += numIncomings;
        in.numProvisionalResp += numProvisionalResp;

        for(Entry<String, Long> entry : incomingMsgMap.entrySet())
        {
            String name = entry.getKey();
            Long value = entry.getValue();

            Long currval = in.incomingMsgMap.get(name);

            if(currval == null)
            {
                in.incomingMsgMap.put(name, value);
            }
            else
            {
                in.incomingMsgMap.put(name, Long.valueOf(currval.longValue() + value.longValue()));
            }
        }

        for(Entry<String, Long> entry : outgoingMsgMap.entrySet())
        {
            String name = entry.getKey();
            Long value = entry.getValue();

            Long currval = in.outgoingMsgMap.get(name);

            if(currval == null)
            {
                in.outgoingMsgMap.put(name, value);
            }
            else
            {
                in.outgoingMsgMap.put(name, Long.valueOf(currval.longValue() + value.longValue()));
            }
        }
    }

    public void loadMessage(SipMsgData msg)
    {
        String name = msg.name;
        if(!msg.isRequest)
        {
            name += (" " + msg.respReq);
        }

        if(msg.isIncoming)
        {
            Long currval = incomingMsgMap.get(name);

            if(currval == null)
            {
                incomingMsgMap.put(name, Long.valueOf(1));
            }
            else
            {
                incomingMsgMap.put(name, Long.valueOf(currval.longValue() + 1));
            }
        }
        else
        {
            Long currval = outgoingMsgMap.get(name);

            if(currval == null)
            {
                outgoingMsgMap.put(name, Long.valueOf(1));
            }
            else
            {
                outgoingMsgMap.put(name, Long.valueOf(currval.longValue() + 1));
            }
        }
    }
}
