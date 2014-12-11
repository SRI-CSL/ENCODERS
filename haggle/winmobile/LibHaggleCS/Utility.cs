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
﻿using System;
using System.Runtime.InteropServices;
using System.Threading;
using System.Diagnostics;

namespace Haggle
{
        public class Utility
        {
                /*
                 * This vibration implementation works for Windows Mobile Professional class devices.
                 * It uses the LED API to turn vibration on and off. The vibration "led" has index 1,
                 *  at least on HTC Touch diamond phones.
                 *
                 * */
                public class Vibration
                {
                        [Flags]
                        public enum LedFlags : int
                        {
                                STATE_OFF = 0x0000,  /* dark LED */
                                STATE_ON = 0x0001,  /* light LED */
                                STATE_BLINK = 0x0002,  /* flashing LED */
                        }

                        public const Int32 NLED_COUNT_INFO_ID = 0;
                        public const Int32 NLED_SUPPORTS_INFO_ID = 1;
                        public const Int32 NLED_SETTINGS_INFO_ID = 2;

                        public class NLED_COUNT_INFO
                        {
                                public UInt32 cLeds;
                        }

                        public class NLED_SUPPORTS_INFO
                        {
                                public UInt32 Lednum;
                                public Int32 lCycleAdjust;
                                public bool fAdjustTotalCycleTime;
                                public bool fAdjustOnTime;
                                public bool fAdjustOffTime;
                                public bool fMetaCycleOn;
                                public bool fMetaCycleOff;
                        };

                        public class NLED_SETTINGS_INFO
                        {
                                public UInt32 LedNum;
                                public LedFlags OnOffBlink;
                                public Int32 TotalCycleTime;
                                public Int32 OnTime;
                                public Int32 OffTime;
                                public Int32 MetaCycleOn;
                                public Int32 MetaCycleoff;
                        };

                        [DllImport("CoreDll")]
                        private extern static bool NLedGetDeviceInfo(Int32 nID, NLED_COUNT_INFO nci);

                        [DllImport("CoreDll")]
                        private extern static bool NLedGetDeviceInfo(Int32 nID, NLED_SUPPORTS_INFO nsi);

                        [DllImport("CoreDll")]
                        private extern static bool NLedGetDeviceInfo(Int32 nID, NLED_SETTINGS_INFO nsi);

                        [DllImport("CoreDll")]
                        private extern static bool NLedSetDevice(Int32 nID, NLED_SETTINGS_INFO nsi);

                        public static uint GetLedCount()
                        {
                                NLED_COUNT_INFO nci = new NLED_COUNT_INFO();

                                uint LedCount = 0;

                                if (NLedGetDeviceInfo(NLED_COUNT_INFO_ID, nci))
                                        LedCount = nci.cLeds;

                                return LedCount;
                        }

                        public static bool SetLedStatus(uint nLed, LedFlags fState)
                        {
                                NLED_SETTINGS_INFO nsi = new NLED_SETTINGS_INFO();

                                nsi.LedNum = nLed;
                                nsi.OnOffBlink = fState;

                                return NLedSetDevice(NLED_SETTINGS_INFO_ID, nsi);
                        }
                        // Seconds to sleep parameter
                        int msecs_vibrate = 1000;
                        int msecs_interval = 500;
                        int num_vibrations = 1;

                        void run()
                        {
                                Debug.WriteLine("Vibrate thread running: vibrating for " + msecs_vibrate + " milliseconds");

                                while (true)
                                {
                                        // Using LED 1 seems to work on at least HTC phones
                                        SetLedStatus(1, LedFlags.STATE_ON);

                                        Thread.Sleep(msecs_vibrate);

                                        Debug.WriteLine("Vibration stop");

                                        SetLedStatus(1, LedFlags.STATE_OFF);

                                        if (--num_vibrations <= 0)
                                        {
                                                break;
                                        }
                                        Thread.Sleep(msecs_interval);
                                }
                        }

                        public void vibrate(int msecs)
                        {
                                vibrate(msecs, msecs_interval, 1);
                        }
                        public void vibrate(int msecs, int msecs_interval)
                        {
                                vibrate(msecs, msecs_interval, 2);
                        }
                        public void vibrate(int msecs, int msecs_interval, int num)
                        {
                                this.msecs_vibrate = msecs;
                                this.msecs_interval = msecs_interval;
                                this.num_vibrations = num;

                                uint ledcnt = GetLedCount();

                                Debug.WriteLine("Led count is " + ledcnt);

                                if (ledcnt <= 0)
                                {
                                        return;
                                }
                                Thread thr = new Thread(new ThreadStart(this.run));

                                thr.Start();
                        }
                }
                /*
                 * The CreateProcess PInvoke code is taken from Microsoft "CompactNac" 
                 * example code and falls under the license associated with that code.
                 * 
                 */
                //class for PROCESSINFO struct pinvoke
                public class ProcessInfo
                {
                        public IntPtr hProcess;
                        public IntPtr hThread;
                        public Int32 ProcessId;
                        public Int32 ThreadId;
                }
                // protoytpe for platform-invoked CreateProcess call
                [DllImport("CoreDll.DLL", SetLastError = true)]
                private extern static int CreateProcess(String imageName,
                    String cmdLine,
                    IntPtr lpProcessAttributes,
                    IntPtr lpThreadAttributes,
                    Int32 boolInheritHandles,
                    Int32 dwCreationFlags,
                    IntPtr lpEnvironment,
                    IntPtr lpszCurrentDir,
                    byte[] si,
                    ProcessInfo pi);

                // CreateProcess wrapper
                public static bool CreateProcess(String ExeName, String
                    CmdLine, ProcessInfo pi)
                {
                        if (pi == null)
                                pi = new ProcessInfo();

                        byte[] si = new byte[128];

                        return CreateProcess(ExeName, CmdLine, IntPtr.Zero,
                            IntPtr.Zero, 0, 0, IntPtr.Zero, IntPtr.Zero, si, pi) != 0;
                }
                public static IntPtr ByteArrayToIntPtr(byte[] b)
                {
                        IntPtr p = Memory.LocalAlloc(Memory.LMEM_FIXED | Memory.LMEM_ZEROINIT, (UInt32)b.Length + 1);

                        if (p == IntPtr.Zero)
                        {
                                throw new OutOfMemoryException();
                        }
                        else
                        {
                                Marshal.Copy(b, 0, p, b.Length);
                        }
                        return p;
                }

                public static byte[] StringToByteArray(string str)
                {
                        System.Text.ASCIIEncoding encoding = new System.Text.ASCIIEncoding();
                        return encoding.GetBytes(str);
                }

                public static string ByteArrayToString(byte[] b)
                {
                        string s;
                        System.Text.ASCIIEncoding enc = new System.Text.ASCIIEncoding();
                        s = enc.GetString(b, 0, b.Length);
                        return s;
                }

                // This will allocate unmanaged memory that should be freed
                public static IntPtr StringToAnsiIntPtr(string s)
                {
                        return ByteArrayToIntPtr(StringToByteArray(s));
                }
                public static string AnsiIntPtrToString(IntPtr p)
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
                /**
                * Convert a string to a UTF-8 encoded byte array.
                * @param str UTF-16 C# string
                * @return UTF-8 multibyte array to be passed to C dll (zero terminated)
                */
                public static byte[] StringToUtf8ByteArray(string str)
                {
                        if (str == null) return new byte[0];
                        byte[] data = System.Text.Encoding.UTF8.GetBytes(str);
                        return data;
                }

                /**
                 * Convert a UTF-8 byte array to a UTF-16 string
                 * @param dBytes UTF-8 multibyte string from C (zero terminated)
                 * @return string to be used in C#
                 */
                public static string Utf8ByteArrayToString(byte[] dBytes)
                {
                        string str;
                        str = System.Text.Encoding.UTF8.GetString(dBytes, 0, dBytes.Length);
                        return str;
                }

                // Creates an IntPtr to an object on the unmanaged heap.
                // Must be freed with Marshal.FreeHGlobal(p) later
                private IntPtr MarshalToPointer(object data)
                {
                        IntPtr p = Marshal.AllocHGlobal(Marshal.SizeOf(data));
                        Marshal.StructureToPtr(data, p, false);
                        return p;
                }
        }

}
