using System;
using System.Linq;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Diagnostics;

namespace LuckyGUI
{
        public class LuckyMeLib
        {
                [DllImport("luckymelib.dll", EntryPoint = "luckyme_test_start")]
                static public extern int startTest();
                [DllImport("luckymelib.dll", EntryPoint = "luckyme_test_stop")]
                static public extern int stopTest();
                [DllImport("luckymelib.dll", EntryPoint = "luckyme_start")]
                static public extern int startLuckyMe();
                [DllImport("luckymelib.dll", EntryPoint = "luckyme_stop")]
                static public extern int stopLuckyMe(int stop_haggle);

                public delegate int SpawnCallback(uint milliseconds);
                [DllImport("luckymelib.dll", EntryPoint = "luckyme_haggle_start")]
                static public extern int startHaggle(SpawnCallback callback);
                [DllImport("luckymelib.dll", EntryPoint = "luckyme_haggle_stop")]
                static public extern int stopHaggle();

                [DllImport("luckymelib.dll", EntryPoint = "luckyme_is_running")]
                static extern int UnmanagedIsLuckyMeRunning();
                public static bool isLuckyMeRunning()
                {
                        return UnmanagedIsLuckyMeRunning() == 1;
                }

                [DllImport("luckymelib.dll", EntryPoint = "luckyme_is_test_running")]
                static extern int UnmanagedIsTestRunning();
                public static bool isTestRunning()
                {
                        return UnmanagedIsTestRunning() == 1;
                }
                [DllImport("luckymelib.dll", EntryPoint = "luckyme_get_num_dataobjects_received")]
                static public extern uint getNumberOfDOsReceived();
                [DllImport("luckymelib.dll", EntryPoint = "luckyme_get_num_dataobjects_created")]
                static public extern uint getNumberOfDOsCreated();
                [DllImport("luckymelib.dll", EntryPoint = "luckyme_get_num_neighbors")]
                static public extern uint getNumberOfNeighbors();
                static string AnsiIntPtrToString(IntPtr p)
                {
                        if (p == IntPtr.Zero)
                                return null;

                        int size = 0;

                        for (; Marshal.ReadByte(p, size) > 0; size++) { }

                        byte[] b = new byte[size];
                        Marshal.Copy(p, b, 0, size);

                        string str = System.Text.Encoding.UTF8.GetString(b, 0, b.Length);

                        return str;
                }

                [DllImport("luckymelib.dll", EntryPoint = "luckyme_neighborlist_lock")]
                static public extern int neighborListLock();

                [DllImport("luckymelib.dll", EntryPoint = "luckyme_neighborlist_unlock")]
                static public extern void neighborListUnlock();

                [DllImport("luckymelib.dll", EntryPoint = "luckyme_get_neighbor_unlocked")]
                static extern IntPtr UnmanagedGetNeighborName(uint n);
                static public String getNeighborName(uint n)
                {
                        IntPtr actual;

                        actual = UnmanagedGetNeighborName(n);
                        if (actual == null)
                        {
                                return null;
                        }
                        return AnsiIntPtrToString(actual);
                }

                [DllImport("libhaggle.dll", EntryPoint = "haggle_daemon_pid")]
                static extern int UnmanagedDaemonPid(ref uint p);
                public static int HagglePid()
                {
                        uint pid = 0;

                        int ret = UnmanagedDaemonPid(ref pid);

                        if (ret == 1)
                                return (int)pid;
                        else
                                return 0;
                }

                public enum HaggleStatus {
                        HAGGLE_DAEMON_ERROR = -100,
                        HAGGLE_DAEMON_NOT_RUNNING = 0,
                        HAGGLE_DAEMON_RUNNING = 1,
                        HAGGLE_DAEMON_CRASHED = 2,
                };

                public static bool isHaggleRunning()
                {
                        uint pid = 0;

                        HaggleStatus ret = (HaggleStatus)UnmanagedDaemonPid(ref pid);

                        //System.Diagnostics.Debug.WriteLine("pid is " + pid);
                        return (ret == HaggleStatus.HAGGLE_DAEMON_RUNNING);
                }
                public static HaggleStatus getHaggleStatus()
                {
                        uint pid = 0;

                        return (HaggleStatus)UnmanagedDaemonPid(ref pid);
                }
              
                public enum EventType
                {
                        EVENT_TYPE_ERROR = -1,
                        EVENT_TYPE_SHUTDOWN = 0,
                        EVENT_TYPE_NEIGHBOR_UPDATE,
                        EVENT_TYPE_NEW_DATAOBJECT,
                        EVENT_TYPE_DATA_OBJECT_GENERATED,
                        EVENT_TYPE_STATUS_UPDATE
                };
                public delegate void UnmanagedCallback(EventType type);
                [DllImport("luckymelib.dll", EntryPoint = "set_callback")]
                static public extern int setCallback(UnmanagedCallback callback);
        }
}
