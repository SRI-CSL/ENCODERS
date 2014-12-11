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
using System.Runtime.InteropServices;

namespace PhotoShare
{
        public class Vibration
        {
                public enum SoundEvent
                {
                        All = 0,
                        RingLine1,
                        RingLine2,
                        KnownCallerLine1,
                        RoamingLine1,
                        RingVoip
                }

                public enum SoundType
                {
                        On = 0,
                        File,
                        Vibrate,
                        None
                }

                public class SoundFileInfo
                {
                        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
                        private string szPathNameNative;
                        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 260)]
                        private string szDisplayNameNative;
                        public SoundType sstType;

                        public string szPathName
                        {
                                get
                                {
                                        return szPathNameNative.Substring(0,
                                            szPathNameNative.IndexOf('\0'));
                                }
                                set { szPathNameNative = value; }
                        }

                        public string szDisplayName
                        {
                                get
                                {
                                        return szDisplayNameNative.Substring(0,
                                            szDisplayNameNative.IndexOf('\0'));
                                }
                                set { szDisplayNameNative = value; }
                        }
                }

                [DllImport("aygshell.dll", SetLastError = true)]
                public static extern uint SndSetSound(SoundEvent seSoundEvent,
                     SoundFileInfo pSoundFileInfo, bool fSuppressUI);

        }
}
