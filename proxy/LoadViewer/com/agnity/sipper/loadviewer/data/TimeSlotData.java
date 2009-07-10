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

    public HashMap<String, Long> msgMap             = new HashMap<String, Long>(25);

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
        msgMap.clear();
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
        
        for(Entry<String, Long> entry : msgMap.entrySet())
        {
            String name = entry.getKey();
            Long value = entry.getValue();
            
            Long currval = in.msgMap.get(name);

            if(currval == null)
            {
                in.msgMap.put(name, value);
            }
            else
            {
                in.msgMap.put(name, Long.valueOf(currval.longValue() + value.longValue()));
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

        Long currval = msgMap.get(name);

        if(currval == null)
        {
            msgMap.put(name, Long.valueOf(1));
        }
        else
        {
            msgMap.put(name, Long.valueOf(currval.longValue() + 1));
        }
    }
}
