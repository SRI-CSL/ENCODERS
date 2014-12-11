using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System.Threading;

namespace LuckyGUI
{
        public partial class TestControlWindow : Form
        {
                long milliseconds_progress = 0;

                public TestControlWindow()
                {
                        InitializeComponent();
                        statusMsgLabel.Text = "";
                        progressBar.Maximum = 60000;

                        if (LuckyMeLib.isHaggleRunning())
                                start_haggle_button.Enabled = false;
                }
                private void updateProgressValue(object sender, EventArgs e)
                {
                        if (milliseconds_progress == 0)
                                progressBar.Value = progressBar.Maximum;
                        else if (milliseconds_progress == -1)
                                progressBar.Value = 0;
                        else
                                progressBar.Value = (int)milliseconds_progress;
                }
                public void updateStartupProgressBar(long milliseconds)
                {
                        milliseconds_progress = milliseconds;
                        progressBar.Invoke(new EventHandler(updateProgressValue));
                }
                public void onShutdown()
                {
                        Debug.WriteLine("testcontrol_window: onShutdown()");
                        statusMsgLabel.Text = "Waiting for Haggle to exit...";
                        this.Refresh();
                }
                public void updateWindowStatus()
                {
                        bool haggleIsRunning = false;
                        statusMsgLabel.Text = "";
                        testStageLabel.Text = "Test stage: " + LuckyMe.testStageString();

                        if (LuckyMeLib.isHaggleRunning())
                        {
                                start_haggle_button.Enabled = false;
                                statusMsgLabel.Text = "Haggle is running";
                                haggleIsRunning = true;
                        }
                        else
                        {
                                start_haggle_button.Enabled = true;
                        }

                        switch (LuckyMe.getTestStage())
                        {
                                case LuckyMe.TestStage.NOT_RUNNING:
                                        start_button.Text = "Start test";
                                        start_button.Enabled = true;
                                        stop_button.Text = "Stop test";
                                        stop_button.Enabled = false;
                                        shutdown_button.Enabled = true;
                                        menuBack.Text = "Back";
                                        updateStartupProgressBar(-1);
                                        progressBar.Visible = false;
                                        break;
                                case LuckyMe.TestStage.STARTING:
                                        start_button.Text = "Please wait...";
                                        start_button.Enabled = false;
                                        statusMsgLabel.Text = "Starting test, please wait...";
                                        stop_button.Text = "Stop test";
                                        stop_button.Enabled = false;
                                        shutdown_button.Enabled = false;
                                        start_haggle_button.Enabled = false;
                                        menuBack.Text = "";
                                        if (!haggleIsRunning)
                                                progressBar.Visible = true;
                                        break;
                                case LuckyMe.TestStage.RUNNING:
                                        start_button.Text = "Start test";
                                        start_button.Enabled = false;
                                        stop_button.Text = "Stop test";
                                        stop_button.Enabled = true;
                                        shutdown_button.Enabled = true;
                                        menuBack.Text = "Back";
                                        updateStartupProgressBar(-1);
                                        progressBar.Visible = false;
                                        start_haggle_button.Enabled = false;
                                        break;
                                case LuckyMe.TestStage.STOPPING:
                                        statusMsgLabel.Text = "Stopping test, please wait...";
                                        start_button.Text = "Start test";
                                        start_button.Enabled = false;
                                        stop_button.Enabled = false;
                                        stop_button.Text = "Please wait...";
                                        shutdown_button.Enabled = false;
                                        menuBack.Text = "";
                                        updateStartupProgressBar(-1);
                                        progressBar.Visible = false;
                                        start_haggle_button.Enabled = false;
                                        break;
                                case LuckyMe.TestStage.SAVING_LOGS:
                                        statusMsgLabel.Text = "Saving log files, please wait...";
                                        start_button.Enabled = false;
                                        stop_button.Enabled = false;
                                        stop_button.Text = "Please wait...";
                                        shutdown_button.Enabled = false;
                                        start_haggle_button.Enabled = false;
                                        menuBack.Text = "";
                                        break;
                                case LuckyMe.TestStage.SHUTDOWN:
                                        progressBar.Visible = false;
                                        statusMsgLabel.Text = "Shutting down...";
                                        start_haggle_button.Enabled = false;
                                        break;
                        }
                        this.Refresh();
                }
                
                private void button_start_Click(object sender, EventArgs e)
                {
                        switch (LuckyMe.getTestStage())
                        {
                                case LuckyMe.TestStage.NOT_RUNNING:
                                        menuBack.Text = "";
                                        start_button.Enabled = false;
                                        start_haggle_button.Enabled = false;
                                        this.Refresh();

                                        if (LuckyMe.startTest())
                                        {
                                                statusMsgLabel.Text = "";
                                        }
                                        else
                                        {
                                                start_button.Enabled = true;
                                                menuBack.Text = "Back";
                                                statusMsgLabel.Text = "Could not start test";
                                        }
                                        this.Refresh();
                                        break;
                                case LuckyMe.TestStage.STARTING:
                                        break;
                                case LuckyMe.TestStage.RUNNING:
                                        break;
                                case LuckyMe.TestStage.STOPPING:
                                default:
                                        break;
                        }
                        Debug.WriteLine("Start button done");
                }

                private void button_stop_Click(object sender, EventArgs e)
                {
                        switch (LuckyMe.getTestStage())
                        {
                                case LuckyMe.TestStage.NOT_RUNNING:
                                case LuckyMe.TestStage.STARTING:
                                        statusMsgLabel.Text = "Could not stop, test not running";
                                        this.Refresh();
                                        break;
                                case LuckyMe.TestStage.RUNNING:
                                        stop_button.Enabled = false;
                                        menuBack.Text = "";
                                        this.Refresh();

                                        if (LuckyMe.stopTest())
                                        {
                                                statusMsgLabel.Text = "";
                                        }
                                        else
                                        {
                                                stop_button.Enabled = true;
                                                menuBack.Text = "Back";
                                                statusMsgLabel.Text = "Could not stop test";
                                        }
                                        this.Refresh();
                                        break;
                                case LuckyMe.TestStage.STOPPING:
                                        statusMsgLabel.Text = "Test is already stopping";
                                        this.Refresh();
                                        break;

                                case LuckyMe.TestStage.SHUTDOWN:
                                        statusMsgLabel.Text = "Shutting down...";
                                        this.Refresh();
                                        break;
                        }
                }

                private void button_shutdown_Click(object sender, EventArgs e)
                {
                        shutdown_button.Enabled = false;
                        start_button.Enabled = false;
                        stop_button.Enabled = false;
                        this.Refresh();

                        if (LuckyMe.shutdown())
                        {
                                Application.Exit();
                        }
                }

                private void menuBack_Click(object sender, EventArgs e)
                {
                        this.Activate();

                        if (LuckyMe.getTestStage() == LuckyMe.TestStage.NOT_RUNNING ||
                                LuckyMe.getTestStage() == LuckyMe.TestStage.RUNNING)
                        {
                                this.DialogResult = DialogResult.OK;
                                this.Close();
                        }
                }

                private void label4_ParentChanged(object sender, EventArgs e)
                {

                }

                private void TestControlWindow_Load(object sender, EventArgs e)
                {

                }

                private void label1_ParentChanged(object sender, EventArgs e)
                {

                }

                private void quit_luckyme_Click(object sender, EventArgs e)
                {
                        if (LuckyMeLib.isLuckyMeRunning())
                        {
                                int res = LuckyMeLib.stopLuckyMe(0);
                        }
                        Application.Exit();
                }

                private void kill_haggle_Click(object sender, EventArgs e)
                {
                        if (LuckyMeLib.isHaggleRunning())
                        {
                                int pid = LuckyMeLib.HagglePid();

                                if (pid != 0)
                                {
                                        try
                                        {
                                                Process haggle = Process.GetProcessById(pid);

                                                haggle.Kill();
                                        }
                                        catch (Exception)
                                        {
                                                Debug.WriteLine("No Haggle process with pid " + pid);
                                        }
                                        Debug.WriteLine("Could not get Haggle process pid");

                                }
                        }
                        else
                        {
                                Debug.WriteLine("Haggle is not running");
                        }
                }

                private void progressBar_ParentChanged(object sender, EventArgs e)
                {

                }

                private void label2_ParentChanged(object sender, EventArgs e)
                {

                }

                private void button_start_Haggle_Click(object sender, EventArgs e)
                {
                        statusMsgLabel.Text = "Starting Haggle...";
                        start_haggle_button.Enabled = false;
                        start_button.Enabled = false;
                        shutdown_button.Enabled = false;
                        progressBar.Visible = true;
                        this.Refresh();
                        LuckyMe.startHaggle();
                }
        }
}