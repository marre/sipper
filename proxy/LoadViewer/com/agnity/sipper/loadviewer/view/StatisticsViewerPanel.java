package com.agnity.sipper.loadviewer.view;

import java.awt.BorderLayout;
import java.awt.FlowLayout;
import java.awt.GridLayout;
import java.util.Timer;
import java.util.TimerTask;

import javax.swing.JLabel;
import javax.swing.JPanel;

import com.agnity.sipper.loadviewer.LoadViewer;
import com.agnity.sipper.loadviewer.data.TimeSlotModalData;

public class StatisticsViewerPanel extends JPanel
{
    private static final long serialVersionUID = 1L;
    Timer                     _timer           = new Timer();
    ViewerTimerTask           _currTimer       = null;

    class ViewerTimerTask extends TimerTask
    {
        int _duration;
        int _refresh;

        class LocTimerTask extends TimerTask
        {
            @Override
            public void run()
            {
                _handleTimer();
            }
        }

        public ViewerTimerTask(int duration, int refresh)
        {
            _duration = duration;
            _refresh = refresh;
        }

        private void _handleTimer()
        {
            if(this != _currTimer) return;

            TimeSlotModalData newData = new TimeSlotModalData();
            newData.duration = _duration;
            newData.refreshDuration = _refresh;
            _parent.getData(newData);

            javax.swing.SwingUtilities.invokeLater(new DataRefresher(newData));
            _timer.schedule(new LocTimerTask(), _refresh * 1000);
        }

        @Override
        public void run()
        {
            _handleTimer();
        }
    }

    class DataRefresher implements Runnable
    {
        TimeSlotModalData _newData;

        public DataRefresher(TimeSlotModalData newData)
        {
            _newData = newData;
        }

        @Override
        public void run()
        {
            _data.activeCalls = _newData.activeCalls;
            _data.activeTransactions = _newData.activeTransactions;
            _data.compData = _newData.compData;
            _data.durationData = _newData.durationData;

            _activeCalls.setText("" + _data.activeCalls);
            _activeTrans.setText("" + _data.activeTransactions);
            _compTable.setDataModal(_data.compData);
            _durationTable.setDataModal(_data.durationData);
        }
    }

    LoadViewer            _parent = null;
    TimeSlotModalData     _data             = null;

    DurationSelectorPanel _refreshSelector  = null;
    DurationSelectorPanel _durationSelector = null;
    StatTableView         _compTable        = null;
    StatTableView         _durationTable    = null;
    JLabel                _activeCalls      = null;
    JLabel                _activeTrans      = null;

    public StatisticsViewerPanel(LoadViewer parent)
    {
        _parent = parent;
        _data             = new TimeSlotModalData();
        _refreshSelector = new DurationSelectorPanel("Refresh", this);
        _durationSelector = new DurationSelectorPanel("Duration", this);
        _compTable = new StatTableView(_data.compData);
        _durationTable = new StatTableView(_data.durationData);
        _activeCalls = new JLabel();
        _activeTrans = new JLabel();
        setLayout(new BorderLayout());
        JPanel topPanel = new JPanel(new GridLayout(3, 1));
        topPanel.add(_refreshSelector);
        topPanel.add(_durationSelector);
        
        JPanel textPanel = new JPanel(new FlowLayout());
        textPanel.add(new JLabel("ActiveCalls"));
        textPanel.add(_activeCalls);
        textPanel.add(new JLabel("ActiveTransactions"));
        textPanel.add(_activeTrans);
        topPanel.add(textPanel);

        JPanel bottomPanel = new JPanel(new BorderLayout());
        bottomPanel.add(_compTable, BorderLayout.WEST);
        bottomPanel.add(_durationTable, BorderLayout.EAST);

        add(topPanel, BorderLayout.NORTH);
        add(bottomPanel, BorderLayout.CENTER);
        
        _refreshSelector.setupButton();
        _durationSelector.setupButton();
    }

    public void handleCommand(String command, int duration)
    {
        if(command.equals("Refresh"))
        {
            _data.refreshDuration = duration;
        }
        else if(command.equals("Duration"))
        {
            _data.duration = duration;
        }

        _currTimer = new ViewerTimerTask(_data.duration, _data.refreshDuration);
        _timer.schedule(_currTimer, 100);
    }
}
