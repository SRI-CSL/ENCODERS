using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Threading;
using System.Diagnostics;


namespace LuckyGUI
{
        public partial class MainWindow : Form
        {
                [DllImport("coredll.dll", EntryPoint = "ShowWindow")]
                static extern int ShowWindow(IntPtr hWnd, int nCmdShow);
                const int SW_MINIMIZED = 6;
                const int SW_SHOW = 5;

                public MainWindow()
                {
                        InitializeComponent();
                        this.Refresh();
                }
               
                public void onNeighborUpdate()
                {
                        Debug.WriteLine("main_window: onNeighborUpdate() stage=" + LuckyMe.testStageString());
                        numNeighborsLabel.Text = LuckyMeLib.getNumberOfNeighbors() + " neighbors";
                        ShowWindow(this.Handle, SW_SHOW);
                        this.Refresh();
                }
                public void onNewDataObject()
                {
                        Debug.WriteLine("main_window: onNewDataObject stage=" + LuckyMe.testStageString());
                        numDataObjectsLabel.Text = LuckyMeLib.getNumberOfDOsReceived() + " data objects received";
                        ShowWindow(this.Handle, SW_SHOW);
                        this.Refresh();
                }
                public void onDataObjectGenerated()
                {
                        Debug.WriteLine("main_window: onDataObjectGenerated stage=" + LuckyMe.testStageString());
                        numDataObjectsGenLabel.Text = LuckyMe.getNumDataObjectsGenerated() + " data objects generated";
                        this.Refresh();
                }

                public void onStatusUpdate()
                {
                        Debug.WriteLine("main_window: onStatusUpdate stage=" + LuckyMe.testStageString());
                        /*
                        onNeighborUpdate();
                        onNewDataObject();
                         */
                        updateWindowStatus();
                }
                public void onShutdown()
                {
                        Debug.WriteLine("main_window: onShutdown() stage=" + LuckyMe.testStageString());
                        haggleStatusLabel.Text = "Waiting for Haggle to exit";
                        numDataObjectsGenLabel.Text = LuckyMe.getNumDataObjectsGenerated() + " data objects generated";
                        this.Refresh();
                }

                private void updateLockStatus()
                {
                        int lock_status = -1;
                        string driverFileName = Microsoft.Win32.Registry.GetValue(@"HKEY_LOCAL_MACHINE\\System\\State", "Lock", "1").ToString();
                        lock_status = Int32.Parse(driverFileName);
                        switch (lock_status)
                        {
                                case 0:
                                        lockStatusLabel.Text = "Phone is unlocked";
                                        lockButton.Enabled = true;
                                        break;

                                case 1:
                                        lockStatusLabel.Text = "Phone is locked (pw)";
                                        lockButton.Enabled = false;
                                        break;

                                case 2:
                                        lockStatusLabel.Text = "Phone is locked";
                                        lockButton.Enabled = false;
                                        ShowWindow(this.Handle, SW_SHOW);
                                        break;

                                default:
                                        lockStatusLabel.Text = "Phone locked(?)";
                                        lockButton.Enabled = false;
                                        break;
                        }
                        this.Refresh();
                }
                public void updateWindowStatus()
                {
                        Debug.WriteLine("main_window: updating status");

                        testStatusLabel.Text = "Test stage: " + LuckyMe.testStageString();

                        LuckyMeLib.HaggleStatus status = LuckyMeLib.getHaggleStatus();

                        switch (status)
                        {
                                case LuckyMeLib.HaggleStatus.HAGGLE_DAEMON_CRASHED:
                                        haggleStatusLabel.Text = "Haggle has crashed";
                                        break;
                                case LuckyMeLib.HaggleStatus.HAGGLE_DAEMON_ERROR:
                                        haggleStatusLabel.Text = "Haggle status error...";
                                        break;
                                case LuckyMeLib.HaggleStatus.HAGGLE_DAEMON_NOT_RUNNING:
                                        haggleStatusLabel.Text = "Haggle is not running";
                                        break;
                                case LuckyMeLib.HaggleStatus.HAGGLE_DAEMON_RUNNING:
                                        haggleStatusLabel.Text = "Haggle is running";
                                        break;
                        }

                        updateLockStatus();

                        if (LuckyMeLib.isLuckyMeRunning())
                        {
                                if (LuckyMeLib.isTestRunning())
                                {
                                        luckyMeStatusLabel.Text = "LuckyMe and test is running";
                                }
                                else
                                {
                                        luckyMeStatusLabel.Text = "LuckyMe is running, but not test";
                                }
                                Debug.WriteLine("main_window: LuckyMe is running");
                        }
                        else
                        {
                                luckyMeStatusLabel.Text = "LuckyMe is not running";
                                Debug.WriteLine("main_window: LuckyMe is not running");
                        }

                        numDataObjectsGenLabel.Text = LuckyMe.getNumDataObjectsGenerated() + " data objects generated";

                        this.Refresh();
                }


                private void menuShowNeighbors_Click(object sender, EventArgs e)
                {
                        DialogResult ret = DialogResult.None;

                        try
                        {
                                ret = LuckyMe.neighbor_window.ShowDialog();
                        }
                        catch (Exception)
                        {
                        }

                        if (ret == DialogResult.OK)
                        {

                        }
                }

                private void menuHide_Click(object sender, EventArgs e)
                {
                        /*
                         * We only hide the application in the background, so
                         * that the user thinks he's quit the application. This way we will
                         * avoid having users quitting the application accidentally during 
                         * the test.
                         * 
                         * To actually quit the application, press the "shutdown" button
                         * in the test controls window.
                        */
                        ShowWindow(this.Handle, SW_MINIMIZED);
                }

                private void controlsMenuItem_Click(object sender, EventArgs e)
                {
                        try
                        {
                                LuckyMe.testcontrol_window.ShowDialog();
                        }
                        catch (Exception)
                        {
                        }
                }

                [DllImport("aygshell.dll")]
                public static extern int SHDeviceLockAndPrompt();

                private void lockButton_Click(object sender, EventArgs e)
                {
                        // Does this work?
                        if (SHDeviceLockAndPrompt() != 0)
                        {
                                System.Diagnostics.Debug.WriteLine("Unable to lock!");
                        }
                        updateLockStatus();
                }

                private void neighborsButton_Click(object sender, EventArgs e)
                {
                        try
                        {
                                LuckyMe.neighbor_window.ShowDialog();
                        }
                        catch (Exception)
                        {
                        }
                }

                private void label6_ParentChanged(object sender, EventArgs e)
                {

                }

                private void MainWindow_KeyDown(object sender, KeyEventArgs e)
                {
                        if ((e.KeyCode == System.Windows.Forms.Keys.Up))
                        {
                                // Up
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.Down))
                        {
                                // Down
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.Left))
                        {
                                // Left
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.Right))
                        {
                                // Right
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.Enter))
                        {
                                // Enter
                        }

                }

                private void numDataObjectsLabel_ParentChanged(object sender, EventArgs e)
                {

                }

                private void numDataObjectsGenLabel_ParentChanged(object sender, EventArgs e)
                {

                }
        }
}