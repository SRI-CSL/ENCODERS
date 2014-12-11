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
using System.Text;
using System.Runtime.InteropServices;
using System.Diagnostics;

namespace Haggle
{
        public class HaggleHandle : IDisposable
        {
                public IntPtr handle;

                [DllImport("libhaggle.dll", EntryPoint = "haggle_handle_get")]
                static extern int UnmanagedGetHandle(IntPtr name, ref IntPtr handlePtr);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_handle_get_session_id")]
                static extern int UnmanagedGetSessionID(IntPtr hh);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_get_error")]
                static extern int UnmanagedGetError();

                [DllImport("libhaggle.dll", EntryPoint = "haggle_handle_free")]
                static extern int UnmanagedFreeHandle(IntPtr hh);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_unregister")]
                static extern int UnmanagedUnregister(IntPtr name);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_ipc_publish_dataobject")]
                static extern int UnmanagedPublishDataObject(IntPtr hh, IntPtr cdObj);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_ipc_add_application_interest")]
                static extern int UnmanagedAddInterest(IntPtr hh, IntPtr name, IntPtr value);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_ipc_add_application_interests")]
                static extern int UnmanagedAddInterests(IntPtr hh, IntPtr al);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_ipc_remove_application_interests")]
                static extern int UnmanagedDeleteInterests(IntPtr hh, IntPtr al);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_ipc_get_application_interests_async")]
                static extern int UnmanagedRequestInterests(IntPtr hh);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_ipc_get_data_objects_async")]
                static extern int UnmanagedRequestDataObjects(IntPtr hh);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_ipc_add_application_interest_weighted")]
                static extern int UnmanagedAddInterestWeighted(IntPtr hh, IntPtr name, IntPtr value, UInt32 weight);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_ipc_delete_data_object")]
                static extern int UnmanagedDeleteDataObject(IntPtr hh, IntPtr cdObj);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_ipc_delete_dataobject_by_id")]
                static extern int UnmanagedDeleteDataObjectById(IntPtr hh, IntPtr id);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_ipc_shutdown")]
                static extern int UnmanagedShutdown(IntPtr hh);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_event_loop_run")]
                static extern int UnmanagedEventLoopRun(IntPtr hh);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_event_loop_run_async")]
                static extern int UnmanagedEventLoopRunAsync(IntPtr hh);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_event_loop_stop")]
                static extern int UnmanagedEventLoopStop(IntPtr hh);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_daemon_pid")]
                static extern int UnmanagedDaemonPid(ref uint pid);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_daemon_spawn")]
                static extern int UnmanagedDaemonSpawn(IntPtr exepath);

                public const int HAGGLE_ERROR = -100;
                public const int HAGGLE_NO_ERROR = 0;
                public const int HAGGLE_BUSY_ERROR = -5;

                public int PublishDataObject(DataObject dObj)
                {
                        return UnmanagedPublishDataObject(this.handle, dObj.cDataObject);
                }
                public int AddInterest(string name, string value)
                {
                        IntPtr nameAnsi = Utility.StringToAnsiIntPtr(name);
                        IntPtr valueAnsi = Utility.StringToAnsiIntPtr(value);

                        int ret = UnmanagedAddInterest(this.handle, nameAnsi, valueAnsi);

                        Memory.LocalFree(nameAnsi);
                        Memory.LocalFree(valueAnsi);
                        return ret;
                }
                public int AddInterest(Haggle.Attribute a)
                {
                        IntPtr nameAnsi = Utility.StringToAnsiIntPtr(a.GetName());
                        IntPtr valueAnsi = Utility.StringToAnsiIntPtr(a.GetValue());

                        int ret = UnmanagedAddInterest(this.handle, nameAnsi, valueAnsi);

                        Memory.LocalFree(nameAnsi);
                        Memory.LocalFree(valueAnsi);
                        return ret;
                }
                public int AddInterest(string name, string value, UInt32 weight)
                {
                        IntPtr nameAnsi = Utility.StringToAnsiIntPtr(name);
                        IntPtr valueAnsi = Utility.StringToAnsiIntPtr(value);

                        int ret = UnmanagedAddInterestWeighted(this.handle, nameAnsi, valueAnsi, weight);

                        Memory.LocalFree(nameAnsi);
                        Memory.LocalFree(valueAnsi);
                        return ret;
                }
                public int AddInterests(Attribute.AttributeList al)
                {
                        return UnmanagedAddInterests(handle, al.cAttrList);
                }

                public void RequestInterests()
                {
                        UnmanagedRequestInterests(handle);
                }
                public void RequestDataObjects()
                {
                        UnmanagedRequestDataObjects(handle);
                }
                public int DeleteInterests(Attribute.AttributeList al)
                {
                        return UnmanagedDeleteInterests(handle, al.cAttrList);
                }
                public int DeleteDataObject(DataObject dObj)
                {
                        return UnmanagedDeleteDataObject(handle, dObj.cDataObject);
                }
                enum DaemonStatus
                {
                        HAGGLE_DAEMON_ERROR = HAGGLE_ERROR,
                        HAGGLE_DAEMON_NOT_RUNNING = HAGGLE_NO_ERROR,
                        HAGGLE_DAEMON_RUNNING = 1,
                        HAGGLE_DAEMON_CRASHED = 2,
                };
                public static uint DaemonPid()
                {
                        uint pid = 0;

                        int ret = UnmanagedDaemonPid(ref pid);

                        if (ret == (int)DaemonStatus.HAGGLE_DAEMON_RUNNING)
                        {
                                Debug.WriteLine("Haggle pid is " + pid);

                                return pid;
                        }
                        return 0;
                }
                public static int SpawnDaemon()
                {
                        return UnmanagedDaemonSpawn(IntPtr.Zero);
                }
                public static int SpawnDaemon(string daemonpath)
                {
                        int ret;
                        IntPtr pathAnsi = Utility.StringToAnsiIntPtr(daemonpath);
                        ret = UnmanagedDaemonSpawn(pathAnsi);
                        Memory.LocalFree(pathAnsi);
                        return ret;
                }
                public int EventLoopRun()
                {
                        return UnmanagedEventLoopRun(handle);
                }
                public int EventLoopRunAsync()
                {
                        return UnmanagedEventLoopRunAsync(handle);
                }
                public int EventLoopStop()
                {
                        return UnmanagedEventLoopStop(handle);
                }
                public int Shutdown()
                {
                        return UnmanagedShutdown(handle);
                }
                public static int Unregister(string name)
                {
                    IntPtr nameAnsi = Utility.StringToAnsiIntPtr(name);

                    int ret = UnmanagedUnregister(nameAnsi);
                    Memory.LocalFree(nameAnsi);

                    return ret;
                }
                public class IPCException : Exception
                {
                        string message;
                        int error;

                        public IPCException(string _message)
                        {
                                message = _message;
                        }
                        public IPCException(string _message, int _error)
                        {
                                message = _message;
                                error = _error;
                        }
                        public override string ToString()
                        {
                                return message;
                        }
                        public int GetError()
                        {
                                return error;
                        }

                }
                public HaggleHandle(string name)
                {
                        IntPtr nameAnsi = Utility.StringToAnsiIntPtr(name);

                        int ret = UnmanagedGetHandle(nameAnsi, ref handle);

                        Memory.LocalFree(nameAnsi);

                        if (ret != HAGGLE_NO_ERROR || handle == IntPtr.Zero)
                        {
                            throw new IPCException("Could not get haggle handle", ret);
                        }

                        Debug.WriteLine("Application " + name + " got haggle handle: " + UnmanagedGetSessionID(handle));
                }

                public void Free()
                {
                        UnmanagedFreeHandle(handle);
                }

                #region IDisposable Members

                public void Dispose()
                {
                        UnmanagedFreeHandle(handle);
                }

                #endregion
        }

        public struct cHaggleEvent
        {
                public uint type;
                public IntPtr data;
        }
        public class HaggleEvent
        {
                public uint type;
                public IntPtr data;
                public const uint SHUTDOWN = 0;
                public const uint NEIGHBOR_UPDATE = 1;
                public const uint NEW_DATAOBJECT = 2;
                public const uint INTEREST_LIST = 3;
                public const uint NUM_EVENTS = 4;
                public HaggleEvent(cHaggleEvent e)
                {
                        this.type = e.type;
                        this.data = e.data;
                }
        };
        public class NeighborEvent : HaggleEvent
        {
                public Node.NodeList neighbors;
                public NeighborEvent(cHaggleEvent e)
                        : base(e)
                {
                        if (e.type != HaggleEvent.NEIGHBOR_UPDATE)
                        {
                                throw new Exception("Not a neighbor update event");
                        }

                        neighbors = new Node.NodeList(e.data);
                }
        };
        public class InterestsEvent : HaggleEvent
        {
                public Attribute.AttributeList interests;
                public InterestsEvent(cHaggleEvent e)
                        : base(e)
                {
                        if (e.type != HaggleEvent.INTEREST_LIST)
                        {
                                throw new Exception("Not an interest list event");
                        }

                        interests = new Attribute.AttributeList(e.data);
                }
        };
        public class DataObjectEvent : HaggleEvent
        {
                public DataObject dObj;
                public DataObjectEvent(cHaggleEvent e)
                        : base(e)
                {
                        if (e.type != HaggleEvent.NEW_DATAOBJECT)
                        {
                                throw new Exception("Not a new data object event");
                        }

                        dObj = new DataObject(e.data);
                }
        };

        public delegate void HaggleCallback(HaggleEvent e);
        public delegate int UnmanagedHaggleCallback(ref cHaggleEvent e, IntPtr arg);
      
        public class HaggleEventHandler
        {
                public event HaggleCallback callback;
                public event UnmanagedHaggleCallback unmanagedCallback;
                public int ret;

                [DllImport("libhaggle.dll", EntryPoint = "haggle_ipc_register_event_interest")]
                public static extern int UnmanagedRegisterEventHandler(IntPtr handle, uint type, UnmanagedHaggleCallback handler, IntPtr arg);
              
                private int UnmanagedHandler(ref cHaggleEvent e, IntPtr arg)
                {
                        Debug.WriteLine("Got haggle event type " + e.type);
                        switch (e.type)
                        {
                                case HaggleEvent.SHUTDOWN:
                                        callback(new HaggleEvent(e));
                                        break;
                                case HaggleEvent.NEIGHBOR_UPDATE:
                                        callback(new NeighborEvent(e));
                                        break;
                                case HaggleEvent.NEW_DATAOBJECT:
                                        callback(new DataObjectEvent(e));
                                        break;
                                case HaggleEvent.INTEREST_LIST:
                                        callback(new InterestsEvent(e));
                                        break;
                                default:
                                        // Do not keep data -> return value != 1
                                        return 0;
                        }
                       
                        // Keep data -> return 1
                        return 1;
                }

                public HaggleEventHandler(HaggleHandle hh, uint type, HaggleCallback callback)
                {
                        if (type < 0 || type >= HaggleEvent.NUM_EVENTS) {
                                throw new EventHandlerException("Bad event type");
                        }
                        this.callback = callback;
                        this.unmanagedCallback = new UnmanagedHaggleCallback(this.UnmanagedHandler);

                        ret = UnmanagedRegisterEventHandler(hh.handle, type, this.unmanagedCallback, IntPtr.Zero);

                        if (ret < 0)
                        {
                                throw new EventHandlerException("Could not register handler.", ret);
                        }

                }
                public class EventHandlerException : Exception
                {
                        string message;
                        int error;

                        public EventHandlerException(string _message)
                        {
                                message = _message;
                        }
                        public EventHandlerException(string _message, int _error)
                        {
                                message = _message;
                                error = _error;
                        }
                        public override string ToString()
                        {
                                return message;
                        }
                        public int GetError()
                        {
                                return error;
                        }

                }
        }
}
