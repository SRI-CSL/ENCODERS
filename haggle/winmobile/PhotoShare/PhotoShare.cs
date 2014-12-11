/* Copyright 2008 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 *     
 *     http://www.apache.org/licenses/LICENSE-2.0 
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */ 
using System;
using System.Linq;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Threading;
using System.IO;
using System.Diagnostics;
using Haggle;
using Microsoft.Win32;
using Microsoft.WindowsMobile;

namespace PhotoShare
{

        public class PhotoShare
        {
                public AddInterestWindow addInterestWindow = new AddInterestWindow();
                public NeighborListWindow neighborListWindow = new NeighborListWindow();
                MainWindow mainWindow;
                public delegate void PhotoListUpdateDelegate(Haggle.DataObject dObj);
                public delegate void NeighborListUpdateDelegate(Node.NodeList neighborList);
                public delegate long InterestListUpdateDelegate(Haggle.Attribute.AttributeList interestList);
                public delegate bool connectToHaggleDelegate();
                HaggleEventHandler updatedNeighborHandler;
                HaggleEventHandler shutdownHandler;
                HaggleEventHandler newDataObjectHandler;
                HaggleEventHandler interestListHandler;
                public HaggleHandle hh;
                DateTime vibrationTime = DateTime.Now;
                Utility.Vibration vib = new Utility.Vibration();

                // The data objects we display in the picture list
                public List<Haggle.DataObject> dataObjects = new List<Haggle.DataObject>();
                public uint numDataObjects = 0;

                // Try to connect to Haggle daemon. If it is not running, ask user if we should try to
                // start it.

                public PhotoShare()
                {
                        if (!connectToHaggle())
                        {
                                Application.Exit();
                                return;
                        }

                        mainWindow = new MainWindow(this);

                        hh.EventLoopRunAsync();

                        Application.Run(mainWindow);
                }
                public void shutdown()
                {
                        hh.Shutdown();
                        hh.Free();
                        Application.Exit();
                }

                public void quit()
                {
                        hh.Free();
                        Debug.WriteLine("Calling Application.Exit()");
                        Application.Exit();
                }
                public bool tryLaunchHaggle()
                {
                        DialogResult res = MessageBox.Show("Haggle does not seem to be running. Start Haggle now?", 
                                "Haggle Error", MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button1);

                        if (res == DialogResult.Yes)
                        {
                                int ret = HaggleHandle.SpawnDaemon();

                                if (ret != 0)
                                {
                                        MessageBox.Show("Could not launch Haggle daemon, error=" + ret);
                                        return false;
                                }
                                return true;
                                // Give Haggle some time to launch
                                //Thread.Sleep(4000);
                        }
                        return false;
                }
                private bool connectToHaggle()
                {
                        bool try_again = true;

                        while (try_again)
                        {
                                try_again = false;

                                if (HaggleHandle.DaemonPid() == 0)
                                {
                                        if (!tryLaunchHaggle())
                                        {
                                                return false;
                                        }
                                }
                                try
                                {
                                        hh = new HaggleHandle("PhotoShare");

                                        interestListHandler = new HaggleEventHandler(hh,
                                              HaggleEvent.INTEREST_LIST, new HaggleCallback(this.onInterestList));

                                        hh.RequestInterests();

                                        updatedNeighborHandler = new HaggleEventHandler(hh,
                                                HaggleEvent.NEIGHBOR_UPDATE, new HaggleCallback(this.onNeighborUpdate));
                                        shutdownHandler = new HaggleEventHandler(hh,
                                               HaggleEvent.SHUTDOWN, new HaggleCallback(this.onHaggleShutdown));
                                        newDataObjectHandler = new HaggleEventHandler(hh,
                                              HaggleEvent.NEW_DATAOBJECT, new HaggleCallback(this.onNewDataObject));

                                        Utility.Vibration vib = new Utility.Vibration();
                                        
                                        Debug.WriteLine("PhotoShare successfully connected to Haggle\n");

                                        return true;
                                }
                                catch (Haggle.HaggleHandle.IPCException ex)
                                {
                                        if (ex.GetError() == HaggleHandle.HAGGLE_BUSY_ERROR)
                                        {
                                                // FIXME: Should really check if there is another photoshare application running...

                                                HaggleHandle.Unregister("PhotoShare");
                                                Thread.Sleep(2000);
                                                try_again = true;
                                        }
                                        else
                                        {
                                                Debug.WriteLine("Haggle Error: " + ex.ToString() + " errnum=" + ex.GetError());
                                                uint pid = HaggleHandle.DaemonPid();

                                                if (pid > 0)
                                                {
                                                        MessageBox.Show("A Haggle PID file exists, but could still not connect to Haggle");
                                                }
                                                else if (pid == 0)
                                                {
                                                        if (!tryLaunchHaggle())
                                                        {
                                                                HaggleHandle.Unregister("PhotoShare");
                                                                return false;
                                                        }
                                                }
                                                else
                                                {
                                                        DialogResult res = MessageBox.Show("Could not connect to Haggle.", 
                                                        "Haggle Error", MessageBoxButtons.OK, MessageBoxIcon.Question, MessageBoxDefaultButton.Button1);

                                                        return false;
                                                }
                                        }
                                }
                                catch (Haggle.HaggleEventHandler.EventHandlerException ex)
                                {
                                        MessageBox.Show(ex.ToString() + " Error=" + ex.GetError());
                                        throw ex;
                                }
                        }

                        Application.Exit();

                        return false;
                }
                // This function will run in the thread called from libhaggle
                private void onNewDataObject(HaggleEvent he)
                {
                        DataObjectEvent e = he as DataObjectEvent;
                        Debug.WriteLine("New Data Object -- metadata:\n" + e.dObj.GetRaw());

                        try
                        {
                                // Verify that the attributes that PhotoShare expects exist
                                e.dObj.GetAttribute("Picture");

                                if (e.dObj.GetFilePath().Length > 0)
                                {
                                        dataObjects.Add(e.dObj);
                                        numDataObjects++;

                                        if ((DateTime.Now - vibrationTime).TotalSeconds > 5)
                                        {
                                                vib.vibrate(300, 100, 2);
                                                vibrationTime = DateTime.Now;
                                        }
                                        mainWindow.photoListView.BeginInvoke(new PhotoListUpdateDelegate(mainWindow.doPhotoListUpdate), e.dObj);
                                }
                        }
                        catch (Haggle.DataObject.NoSuchAttributeException)
                        {
                                Debug.WriteLine("No Picture attribute in received data object");
                        }
                }

                public void onNeighborUpdate(HaggleEvent he)
                {
                        NeighborEvent e = he as NeighborEvent;
                        //Vibration.SoundFileInfo sfi = new Vibration.SoundFileInfo();
                        //sfi.sstType = Vibration.SoundType.Vibrate;
                        Debug.WriteLine("Neighbor update event!");

                        //uint ret = Vibration.SndSetSound(Vibration.SoundEvent.All, sfi, true);

                        Debug.WriteLine("neighbors received");

                        if ((DateTime.Now - vibrationTime).TotalSeconds > 5)
                        {
                                vib.vibrate(100, 50, 3);
                                vibrationTime = DateTime.Now;
                        }

                        neighborListWindow.BeginInvoke(new NeighborListUpdateDelegate(neighborListWindow.doNeighborListUpdate), e.neighbors);
                }

                private void onHaggleShutdown(HaggleEvent e)
                {
                        MessageBox.Show("Haggle daemon was shut down");
                }
                private void onInterestList(HaggleEvent he)
                {
                        InterestsEvent e = he as InterestsEvent;
                        addInterestWindow.BeginInvoke(new InterestListUpdateDelegate(addInterestWindow.interestListUpdate), e.interests);
                }
                /// <summary>
                /// The main entry point for the application.
                /// </summary>
                [MTAThread]
                static void Main()
                {

                        try
                        {
                                new PhotoShare();
                        }
                        catch
                        {
                        }
                }
        }
}
