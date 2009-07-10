package com.agnity.sipper.loadviewer;

import javax.swing.JFrame;

import com.agnity.sipper.loadviewer.data.SipMsgData;
import com.agnity.sipper.loadviewer.data.TimeSlotModalData;
import com.agnity.sipper.loadviewer.view.StatisticsViewerPanel;

public class LoadViewer
{
    SockDataReader  _reader;
    LoadDataManager _dataManager;

    public LoadViewer(String ip, short port) throws Exception
    {
        _reader = new SockDataReader(ip, port, this);
        _dataManager = new LoadDataManager();

        javax.swing.SwingUtilities.invokeLater(new Runnable() {
            public void run()
            {
                try
                {
                    _displayStart();
                }
                catch(Exception e)
                {
                    e.printStackTrace();
                }
            }
        });

        new Thread() {
            public void run()
            {
                _reader.startRead();
            }
        }.start();

        while(true)
        {
            Thread.sleep(5000);
        }
    }

    public static void main(String[] args) throws Exception
    {
        new LoadViewer(args[0], Short.parseShort(args[1]));
    }

    public void processIncomingMsg(SipMsgData msg)
    {
        _dataManager.loadData(msg);
    }

    public void getData(TimeSlotModalData data)
    {
        _dataManager.getLoadData(data);
    }

    private void _displayStart()
    {
        JFrame.setDefaultLookAndFeelDecorated(true);
        JFrame frame = new JFrame("Statistics viewer");
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

        frame.setContentPane(new StatisticsViewerPanel(this));

        frame.pack();
        frame.setVisible(true);
    }
}
