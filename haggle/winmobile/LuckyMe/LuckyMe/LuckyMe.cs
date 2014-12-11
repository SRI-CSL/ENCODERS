using System;
using System.Linq;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Threading;
using System.Diagnostics;

namespace LuckyGUI
{
        static class LuckyMe
        {
                public enum TestStage
                {
                        NOT_RUNNING = 0,
                        STARTING,
                        RUNNING,
                        STOPPING,
                        SAVING_LOGS,
                        SHUTDOWN
                };
                public static string[] testStageStrings = 
                { "NOT RUNNING", "STARTING", "RUNNING", "STOPPING", "SAVING LOGS", "SHUTDOWN" };
                private static TestStage testStage = TestStage.NOT_RUNNING;
                public static NeighborWindow neighbor_window = new NeighborWindow();
                public static TestControlWindow testcontrol_window = new TestControlWindow();
                public static MainWindow main_window = new MainWindow();
                public delegate void myDelegate();
                public delegate void myDelegateState(object state);
                private static LuckyMeLib.UnmanagedCallback mCallback = null;
                private static LuckyMeLib.SpawnCallback mSpawnCallback = null;
                public static bool inShutdown = false; 
                private static ulong numDataObjectsGenerated = 0;
                private static System.Threading.Timer mCallTimer = null;

                public static string testStageString()
                {
                        return testStageStrings[(int)testStage];
                }
                private static void updateWindowStatus()
                {
                        if (main_window.InvokeRequired)
                        {
                                main_window.BeginInvoke(new myDelegate(main_window.updateWindowStatus));
                        }
                        else
                        {
                                main_window.updateWindowStatus();
                        }
                        if (testcontrol_window.InvokeRequired)
                        {
                                testcontrol_window.BeginInvoke(new myDelegate(testcontrol_window.updateWindowStatus));
                        }
                        else
                        {
                                testcontrol_window.updateWindowStatus();
                        }
                }
                public static void setTestStage(TestStage stage)
                {
                        if (stage == testStage)
                                return;

                        testStage = stage;

                        updateWindowStatus();
                }
                public static TestStage getTestStage()
                {
                        return testStage;
                }
                public static ulong getNumDataObjectsGenerated()
                {
                        return numDataObjectsGenerated;
                }

                public static int spawnCallback(uint milliseconds)
                {
                        Debug.WriteLine("Spawning, milliseconds=" + milliseconds);

                        testcontrol_window.updateStartupProgressBar((long)milliseconds);

                        return 0;
                }
                public static void eventCallback(LuckyMeLib.EventType eventType)
                {
                        bool exitApplication = false;
                        DialogResult res;

                        // Make sure we execute the callbacks in the Window thread
                        switch (eventType)
                        {
                                case LuckyMeLib.EventType.EVENT_TYPE_ERROR:
                                        Debug.WriteLine("main_window: got error event");
                                        res = MessageBox.Show("An error occurred in LuckyMe. Exit LuckyMe?", "", MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button1);

                                        if (res == DialogResult.Yes)
                                        {
                                                exitApplication = true;

                                                if (LuckyMeLib.isLuckyMeRunning())
                                                {
                                                        LuckyMeLib.stopLuckyMe(0);
                                                }
                                        }
                                        break;
                                case LuckyMeLib.EventType.EVENT_TYPE_SHUTDOWN:
                                        Debug.WriteLine("callback: Haggle was shutdown");

                                        LuckyMeLib.stopLuckyMe(0);

                                        if (LuckyMe.getTestStage() == LuckyMe.TestStage.SHUTDOWN)
                                        {
                                                Debug.WriteLine("main_window: calling Application.Exit()");
                                                exitApplication = true;
                                        }
                                        else
                                        {
                                                Debug.WriteLine("main_window: got shutdown event, but is not in shutdown stage");

                                                res = MessageBox.Show("Haggle was shutdown. Exit LuckyMe?", "", MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button1);

                                                if (res == DialogResult.Yes)
                                                {
                                                        exitApplication = true;
                                                }
                                        }

                                        main_window.BeginInvoke(new myDelegate(main_window.onShutdown));
                                        testcontrol_window.BeginInvoke(new myDelegate(testcontrol_window.onShutdown));
                                        uint n = 0;
                                        // Wait for Haggle to shutdown
                                        while (LuckyMeLib.isHaggleRunning() && n++ < 120)
                                        {
                                                Debug.WriteLine("Waiting for Haggle to exit...");
                                                Thread.Sleep(1000);
                                        }
                                        Debug.WriteLine("Saving log files");
                                        setTestStage(TestStage.SAVING_LOGS);
                                        archiveHaggleFiles(true);
                                        setTestStage(TestStage.NOT_RUNNING);

                                        Debug.WriteLine("Log files saved");

                                        break;
                                case LuckyMeLib.EventType.EVENT_TYPE_NEIGHBOR_UPDATE:
                                        Debug.WriteLine("callback neighbor update");
                                        main_window.BeginInvoke(new myDelegate(main_window.onNeighborUpdate));
                                        neighbor_window.BeginInvoke(new myDelegate(neighbor_window.onNeighborUpdate));
                                        break;
                                case LuckyMeLib.EventType.EVENT_TYPE_NEW_DATAOBJECT:
                                        Debug.WriteLine("callback new data object");
                                        main_window.BeginInvoke(new myDelegate(main_window.onNewDataObject));
                                        break;
                                case LuckyMeLib.EventType.EVENT_TYPE_DATA_OBJECT_GENERATED:
                                        Debug.WriteLine("callback data object generated");
                                        numDataObjectsGenerated++;
                                        main_window.BeginInvoke(new myDelegate(main_window.onDataObjectGenerated));
                                        break;
                                case LuckyMeLib.EventType.EVENT_TYPE_STATUS_UPDATE:
                                        Debug.WriteLine("callback status update");
                                        main_window.BeginInvoke(new myDelegate(main_window.onStatusUpdate));
                                        break;
                        }

                        if (exitApplication)
                                Application.Exit();
                }

                public static bool startHaggle()
                {
                        // Check Haggle status
                        LuckyMeLib.HaggleStatus status = LuckyMeLib.getHaggleStatus();

                        if (status != LuckyMeLib.HaggleStatus.HAGGLE_DAEMON_RUNNING)
                        {

                                Debug.WriteLine("Deleting old Haggle files");
                                // Make sure we delete old haggle files.
                                archiveHaggleFiles(false);

                                int res = LuckyMeLib.startHaggle(mSpawnCallback);

                                if (res < 0)
                                {
                                        Debug.WriteLine("startTest: startHaggle returned res=" + res);
                                        return false;
                                }

                                status = LuckyMeLib.getHaggleStatus();

                                if (status != LuckyMeLib.HaggleStatus.HAGGLE_DAEMON_RUNNING)
                                {
                                        Debug.WriteLine("startTest: Haggle deamon is not running");
                                        return false;
                                }
                                // Let Haggle do its thing for a while before we try to connect
                                Thread.Sleep(5000);
                        }

                        // Start status check timer
                        mCallTimer.Change(1000, 8000);

                        updateWindowStatus();

                        return true;
                }
                public static bool startTest()
                {
                        if (testStage != TestStage.NOT_RUNNING)
                        {
                                return false;
                        }

                        // Reset counters
                        numDataObjectsGenerated = 0;

                        setTestStage(TestStage.STARTING);

                        if (!startHaggle())
                        {
                                setTestStage(TestStage.NOT_RUNNING);
                                return false;
                        }

                        // Start LuckyMe thread
                        if (!LuckyMeLib.isLuckyMeRunning())
                        {
                                int res = LuckyMeLib.startLuckyMe();

                                if (res < 0)
                                {
                                        Debug.WriteLine("Error: Could not start LuckyMe!");
                                        setTestStage(TestStage.NOT_RUNNING);
                                        return false;
                                }

                                // Check again that LuckyMe and the Haggle event loop is running
                                if (!LuckyMeLib.isLuckyMeRunning())
                                {
                                        Debug.WriteLine("Error: LuckyMe not running after start!");
                                        setTestStage(TestStage.NOT_RUNNING);
                                        return false;
                                }
                        }
                        else
                        {
                                Debug.WriteLine("LuckyMe already running");
                        }

                        Thread.Sleep(2000);

                        if (!LuckyMeLib.isTestRunning())
                        {
                                Debug.WriteLine("Starting LuckyMe test loop");

                                int res = LuckyMeLib.startTest();

                                if (res < 0)
                                {
                                        Debug.WriteLine("Error: Could not start LuckyMe test loop!");
                                        setTestStage(TestStage.NOT_RUNNING);
                                        LuckyMeLib.stopHaggle();
                                        return false;
                                }
                        }
                        else
                        {
                                Debug.WriteLine("LuckyMe test already running");
                        }

                        setTestStage(TestStage.RUNNING);

                        return true;
                }
                /*
                 *  Calling shutdown is a more forcefull way of
                 *  killing off LuckyMe and Haggle.
                 *  Returns: true if the caller should call Application.exit()
                 *  or, false if Application.exit() will be called automatically.
                 * */
                public static bool shutdown()
                {
                        bool retval = true;
                        setTestStage(TestStage.SHUTDOWN);

                        if (LuckyMeLib.isHaggleRunning() && LuckyMeLib.isLuckyMeRunning())
                        {
                                Debug.WriteLine("shutdown: Trying to stop LuckyMe and Haggle");
                                LuckyMeLib.stopLuckyMe(1);
                                retval = false;
                        }
                        else
                        {
                                if (LuckyMeLib.isLuckyMeRunning())
                                {
                                        Debug.WriteLine("shutdown: Trying to stop LuckyMe");
                                        LuckyMeLib.stopLuckyMe(0);
                                }
                                if (LuckyMeLib.isHaggleRunning())
                                {
                                        Debug.WriteLine("shutdown: Trying to stop Haggle");
                                        LuckyMeLib.stopHaggle();
                                }
                        }
                        
                        Debug.WriteLine("shutdown: completed...");
                        return retval;
                }

                public static bool stopTest()
                {
                        bool retval = true; // be optimistic :)

                        if (testStage != TestStage.RUNNING)
                        {
                                Debug.WriteLine("Cannot stop test since test stage=" + testStageString());
                                return false;
                        }

                        mCallTimer.Change(System.Threading.Timeout.Infinite, System.Threading.Timeout.Infinite);

                        setTestStage(TestStage.STOPPING);

                        if (LuckyMeLib.isLuckyMeRunning())
                        {
                                if (LuckyMeLib.isHaggleRunning())
                                {
                                        int res = LuckyMeLib.stopLuckyMe(1);

                                        Debug.WriteLine("stopLuckyMe() returned " + res);

                                        if (res < 0)
                                        {
                                                Debug.WriteLine("stopTest(): stopLuckyMe failed");

                                                if (LuckyMeLib.isTestRunning())
                                                {
                                                        setTestStage(TestStage.RUNNING);
                                                }
                                                else
                                                {
                                                        // Set not running although test stop failed
                                                        setTestStage(TestStage.NOT_RUNNING);
                                                        return true;
                                                }

                                                return false;
                                        }

                                        setTestStage(TestStage.STOPPING);

                                }
                                else
                                {
                                        Debug.WriteLine("stopTest(): Haggle was not running");
                                        int res = LuckyMeLib.stopLuckyMe(0);
                                        // Haggle was not running so we cannot expect a shutdown callback
                                        // -> set NOT_RUNNING immediately
                                        setTestStage(TestStage.NOT_RUNNING);
                                }
                        }
                        else
                        {
                                if (LuckyMeLib.isHaggleRunning())
                                {
                                        int res = LuckyMeLib.stopHaggle();

                                        if (res == 1)
                                        {
                                                Debug.WriteLine("stopTest(): Haggle stopped");
                                        }
                                        else if (res == -1)
                                        {
                                                Debug.WriteLine("stopTest(): Could not stop Haggle, no Haggle handle");
                                        }
                                        else
                                        {
                                                Debug.WriteLine("stopTest(): Could not stop Haggle, not running");
                                        }
                                }

                                setTestStage(TestStage.NOT_RUNNING);
                        }
                        return retval;
                }

                public static void onTimerStatusCheck(object state)
                {
                        Debug.WriteLine("Timer status check: Checking if Haggle is still running");

                        LuckyMeLib.HaggleStatus status = LuckyMeLib.getHaggleStatus();

                        switch (status) {
                                case LuckyMeLib.HaggleStatus.HAGGLE_DAEMON_NOT_RUNNING:
                                        Debug.WriteLine("status check: Haggle is NOT running");

                                        if (getTestStage() == TestStage.RUNNING)
                                        {
                                                main_window.BeginInvoke(new myDelegate(main_window.updateWindowStatus));
                                        }
                                        stopTest();
                                        break;
                                case LuckyMeLib.HaggleStatus.HAGGLE_DAEMON_CRASHED:
                                        Debug.WriteLine("status check: Haggle daemon crashed");
                                        main_window.BeginInvoke(new myDelegate(main_window.updateWindowStatus));

                                        stopTest();

                                        DialogResult res = MessageBox.Show("Haggle crashed! Exit LuckyMe?", "", MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button1);


                                        if (res == DialogResult.Yes)
                                        {
                                                Application.Exit();
                                        }
                                        break;
                                case LuckyMeLib.HaggleStatus.HAGGLE_DAEMON_ERROR:
                                        Debug.WriteLine("status check: Haggle daemon error");
                                        break;
                                case LuckyMeLib.HaggleStatus.HAGGLE_DAEMON_RUNNING:
                                        Debug.WriteLine("status check: Haggle daemon running");
                                        break;
                                default:
                                        Debug.WriteLine("status check: Unknown status=" + status);
                                        break;
                        }
                }
              
                private static void killHaggle()
                {
                        System.Diagnostics.Process clsProcess;
                        clsProcess = System.Diagnostics.Process.GetProcessById(LuckyMeLib.HagglePid());
                        if (clsProcess != null)
                        {
                                try
                                {
                                        //clsProcess.Kill();
                                }
                                catch (Exception e)
                                {
                                        System.Diagnostics.Debug.WriteLine("Exception: " + e);
                                }
                        }
                }


                /*
                 * This function can be called for two reasons:
                 * 1) Normal operation: save the data base and move the old files aside.
                 * 2) Abort test: delete all data.
                 * 
                 * 2) is only called in response to "abort test". This is necessary if 
                 * the user accidentally started haggle before starting luckyGUI, or 
                 * something.
                 */
                private static bool archiveHaggleFiles(bool saveFiles)
                {

                        if (LuckyMeLib.isTestRunning())
                        {
                                Debug.WriteLine("archiveHaggleFiles(): Test is still running...");
                                return false;
                        }
                       
                        int i;

                        // 2) store haggle folder on SD card:
                        // FIXME: "\Application Data\" should be "%APPDATA%"
                        string sourcedir = "\\Carte de Stockage\\Haggle";
                        string source2dir = "\\Application Data\\Haggle";
                        // FIXME: "\Storage Card\" should probably be something like "%STORAGECARD%"
                        string archivedirbase = "\\Carte de Stockage\\Haggle-Backup";

                        if (System.IO.Directory.Exists(source2dir))
                        {
                                string[] file = System.IO.Directory.GetFiles(source2dir);
                                for (i = 0; i < file.Length; i++)
                                {
                                       
                                        try
                                        {
                                                if (saveFiles && System.IO.Directory.Exists(sourcedir))
                                                {
                                                        string dstfile = sourcedir + "\\" + System.IO.Path.GetFileName(file[i]);
                                                        Debug.WriteLine("Copy: " + file[i] + " to " + dstfile);
                                                        System.IO.File.Copy(file[i], dstfile, true);
                                                }
                                                // Delete all files that are not libhaggle.txt (because it's open) and
                                                // startup.do, because we want it to stay put.
                                                if (!(System.IO.Path.GetFileName(file[i]).Equals("libhaggle.txt") ||
                                                        System.IO.Path.GetFileName(file[i]).Equals("startup.do") ||
                                                        System.IO.Path.GetFileName(file[i]).Equals("config.xml")))
                                                {
                                                        Debug.WriteLine("Delete: " + file[i]);
                                                        System.IO.File.Delete(file[i]);
                                                }
                                        }
                                        catch (Exception e)
                                        {
                                                System.Diagnostics.Debug.WriteLine("Error: " + e);
                                                return false;
                                        }
                                }
                        }
                        string archivedir;
                        i = 0;
                        do
                        {
                                archivedir = archivedirbase + "-" + i;
                                i++;
                        } while (System.IO.Directory.Exists(archivedir));

                        try
                        {
                                if (saveFiles)
                                {
                                        if (System.IO.Directory.Exists(sourcedir))
                                        {
                                                Debug.WriteLine("Move: " + sourcedir + " to " + archivedir);
                                                System.IO.Directory.Move(sourcedir, archivedir);
                                        }
                                }
                                else
                                {
                                        if (System.IO.Directory.Exists(source2dir))
                                        {
                                                string[] file = System.IO.Directory.GetFiles(source2dir);
                                                for (i = 0; i < file.Length; i++)
                                                {
                                                        if (!(System.IO.Path.GetFileName(file[i]).Equals("libhaggle.txt") ||
                                                                System.IO.Path.GetFileName(file[i]).Equals("startup.do") ||
                                                                System.IO.Path.GetFileName(file[i]).Equals("config.xml")))
                                                        {
                                                                Debug.WriteLine("Delete: " + file[i]);
                                                                System.IO.File.Delete(file[i]);
                                                        }
                                                }
                                        }
                                        if (System.IO.Directory.Exists(sourcedir))
                                        {
                                                string[] file = System.IO.Directory.GetFiles(sourcedir);
                                                for (i = 0; i < file.Length; i++)
                                                {
                                                        Debug.WriteLine("Delete: " + file[i]);
                                                        System.IO.File.Delete(file[i]);
                                                }
                                        }
                                }
                        }
                        catch (Exception)
                        {
                                System.Diagnostics.Debug.WriteLine("Could not move " + sourcedir);
                        }
                        return true;
                }

                /// <summary>
                /// The main entry point for the application.
                /// </summary>
                [MTAThread]
                static void Main()
                {

                        mCallback = new LuckyMeLib.UnmanagedCallback(eventCallback);
                        mSpawnCallback = new LuckyMeLib.SpawnCallback(spawnCallback);
                        mCallTimer = new System.Threading.Timer(new TimerCallback(new myDelegateState(onTimerStatusCheck)),
                                       null, System.Threading.Timeout.Infinite, System.Threading.Timeout.Infinite);

                        if (LuckyMeLib.setCallback(mCallback) != 1)
                        {
                                System.Diagnostics.Debug.WriteLine("Could not set callback function");
                        }

                        if (LuckyMeLib.isHaggleRunning())
                        {
                                mCallTimer.Change(1000, 8000);
                                int res = LuckyMeLib.startLuckyMe();

                                if (res < 0)
                                {
                                        Debug.WriteLine("Could not start LuckyMe");
                                }
                                Debug.WriteLine("Started LuckyMe...");
                        }

                        main_window.updateWindowStatus();

                        Application.Run(main_window);
                        LuckyMeLib.stopLuckyMe(0);
                }
        }
}