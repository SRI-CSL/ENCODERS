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
using System.Linq;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Text;
using System.Diagnostics;

namespace Haggle
{
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
        public class DataObject : IDisposable
        {
                public IntPtr cDataObject;
                static ulong counter = 0;
                ulong num;

                [DllImport("libhaggle.dll", EntryPoint = "haggle_dataobject_new_from_file")]
                static extern IntPtr UnmanagedNewFromFile(IntPtr fileName);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_dataobject_new")]
                static extern IntPtr UnmanagedNew();

                [DllImport("libhaggle.dll", EntryPoint = "haggle_dataobject_free")]
                static extern void UnmanagedFree(IntPtr dObj);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_dataobject_get_raw")]
                static extern IntPtr UnmanagedGetRaw(IntPtr dObj);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_dataobject_get_filename")]
                static extern IntPtr UnmanagedGetFileName(IntPtr dObj);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_dataobject_get_filepath")]
                static extern IntPtr UnmanagedGetFilePath(IntPtr dObj);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_dataobject_get_num_attributes")]
                static extern UInt32 UnmanagedGetNumAttributes(IntPtr dObj);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_dataobject_add_attribute")]
                static extern int UnmanagedAddAttribute(IntPtr dObj, IntPtr name, IntPtr value);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_dataobject_add_hash")]
                static extern int UnmanagedAddHash(IntPtr dObj);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_dataobject_set_thumbnail")]
                static extern IntPtr UnmanagedSetThumbnail(IntPtr dObj, IntPtr data, UInt32 len);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_dataobject_get_attribute_n")]
                static extern IntPtr UnmanagedGetAttributeN(IntPtr dObj, UInt32 n);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_dataobject_get_attribute_by_name")]
                static extern IntPtr UnmanagedGetAttributeByName(IntPtr dObj, IntPtr name);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_dataobject_get_attribute_by_name_n")]
                static extern IntPtr UnmanagedGetAttributeByNameN(IntPtr dObj, IntPtr name, UInt32 n);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_dataobject_get_attribute_by_name_value")]
                static extern IntPtr UnmanagedGetAttributeByNameValue(IntPtr dObj, IntPtr name, IntPtr value);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_dataobject_get_attributelist")]
                static extern IntPtr UnmanagedGetAttributeList(IntPtr dObj);

                public class DataObjectException : Exception
                {
                        string msg;

                        public DataObjectException(string _msg)
                        {
                                msg = _msg;
                        }
                        public override string ToString()
                        {
                                return msg;
                        }
                }
                public class NoSuchAttributeException : Exception
                {
                }
                public DataObject(IntPtr cDataObject)
                {
                        if (cDataObject == IntPtr.Zero)
                        {
                                throw new DataObjectException("Could not create data object");
                        }
                        num = counter;
                        counter++;
                        this.cDataObject = cDataObject;
                        Debug.WriteLine("Created data object " + num);
                }
                public DataObject(string _fileName)
                {
                        IntPtr fileNameAnsi = Utility.StringToAnsiIntPtr(_fileName);
                        this.cDataObject = UnmanagedNewFromFile(fileNameAnsi);
                        Memory.LocalFree(fileNameAnsi);

                        if (this.cDataObject == IntPtr.Zero)
                        {
                                throw new DataObjectException("Could not created data object");
                        }
                        num = counter;
                        counter++;

                        Debug.WriteLine("Created data object " + num);
                }

                public DataObject()
                {
                        this.cDataObject = UnmanagedNew();

                        if (this.cDataObject == IntPtr.Zero)
                        {
                                throw new DataObjectException("Could not created data object");
                        }
                        num = counter;
                        counter++;

                        Debug.WriteLine("Created data object " + num);
                }

                ~DataObject() 
                {
                        Debug.WriteLine("Freeing data object " + num + ", num left=" + (counter - 1));
                        if (this.cDataObject != IntPtr.Zero)
                                UnmanagedFree(this.cDataObject);
                        counter--;
                }
                public void SetThumbnail(byte[] data)
                {
                        UnmanagedSetThumbnail(this.cDataObject, Utility.ByteArrayToIntPtr(data), (uint) data.Length);
                }
                public string GetRaw()
                {
                        return Utility.AnsiIntPtrToString(UnmanagedGetRaw(this.cDataObject));
                }
                public void PrintAttributes()
                {
                        UInt32 i = 0;

                        while (true)
                        {
                                IntPtr a;
                                Haggle.Attribute attr;

                                a = UnmanagedGetAttributeN(this.cDataObject, i++);

                                if (a == IntPtr.Zero)
                                {
                                        //Debug.WriteLine("UnmanagedAttribute zero pointer");
                                        break;
                                }

                                attr = new Attribute(a, false);

                                Debug.WriteLine("Attribute" + attr.GetName() + ":" + attr.GetValue());
                        }
                }
                public bool AddAttribute(string name, string value)
                {
                        IntPtr nameAnsi = Utility.StringToAnsiIntPtr(name);
                        IntPtr valueAnsi = Utility.StringToAnsiIntPtr(value);

                        int ret = UnmanagedAddAttribute(this.cDataObject, nameAnsi, valueAnsi);

                        Memory.LocalFree(nameAnsi);
                        Memory.LocalFree(valueAnsi);

                        if (ret > 0)
                                return true;

                        return false;
                }
                public bool AddHash()
                {
                        int ret = UnmanagedAddHash(this.cDataObject);

                        if (ret < 0) {
                                Debug.WriteLine("Could not add file hash");
                                return false;
                        }

                        return true;
                }
                public bool AddAttribute(Attribute a)
                {
                        IntPtr nameAnsi = Utility.StringToAnsiIntPtr(a.GetName());
                        IntPtr valueAnsi = Utility.StringToAnsiIntPtr(a.GetValue());

                        int ret = UnmanagedAddAttribute(this.cDataObject, nameAnsi, valueAnsi);

                        Memory.LocalFree(nameAnsi);
                        Memory.LocalFree(valueAnsi);

                        if (ret > 0)
                                return true;

                        return false;
                }
                public Attribute GetAttribute(UInt32 n)
                {
                        Attribute attr;

                        try
                        {
                                attr = new Attribute(UnmanagedGetAttributeN(this.cDataObject, n), false);
                        }
                        catch (NullReferenceException)
                        {
                                throw new NoSuchAttributeException();
                        }
                        return attr;
                }
                public Attribute GetAttribute(string name)
                {
                        IntPtr nameAnsi = Utility.StringToAnsiIntPtr(name);
                        Attribute attr;

                        try
                        {
                                attr = new Attribute(UnmanagedGetAttributeByName(this.cDataObject, nameAnsi), false);
                        }
                        catch (NullReferenceException)
                        {
                                throw new NoSuchAttributeException();
                        }
                        finally
                        {
                                Memory.LocalFree(nameAnsi);
                        }

                        Memory.LocalFree(nameAnsi);
                        return attr;
                }
                public Attribute GetAttribute(string name, UInt32 n)
                {
                        IntPtr nameAnsi = Utility.StringToAnsiIntPtr(name);
                        Attribute attr;
                        try
                        {
                                attr = new Attribute(UnmanagedGetAttributeByNameN(this.cDataObject, nameAnsi, n), false);
                        }
                        catch (NullReferenceException)
                        {
                                throw new NoSuchAttributeException();
                        }
                        finally 
                        {
                                Memory.LocalFree(nameAnsi);
                        }
                        Memory.LocalFree(nameAnsi);
                        return attr;
                }
                public Attribute[] GetAttributes(string name)
                {
                        System.Collections.ArrayList arr = new System.Collections.ArrayList();
                        IntPtr nameAnsi = Utility.StringToAnsiIntPtr(name); 
                        UInt32 i = 0;

                        while (true)
                        {
                                IntPtr cAttr;
                                Haggle.Attribute a;

                                cAttr = UnmanagedGetAttributeByNameN(this.cDataObject, nameAnsi, i++);

                                if (cAttr == IntPtr.Zero) {
                                        //Debug.WriteLine("UnmanagedAttribute zero pointer");
                                        break;
                                }

                                a = new Attribute(cAttr, false);

                                arr.Add(a);
                        }
                        return (Attribute[])arr.ToArray(typeof(Attribute));
                }
                public Attribute GetAttribute(string name, string value)
                {
                        IntPtr nameAnsi = Utility.StringToAnsiIntPtr(name);
                        IntPtr valueAnsi = Utility.StringToAnsiIntPtr(value);
                        Attribute attr;

                        try
                        {
                                attr = new Attribute(UnmanagedGetAttributeByNameValue(this.cDataObject, nameAnsi, valueAnsi), false);
                        }
                        catch (NullReferenceException)
                        {
                                throw new NoSuchAttributeException();
                        }
                        finally
                        {
                                Memory.LocalFree(nameAnsi);
                                Memory.LocalFree(valueAnsi);
                        }
                        Memory.LocalFree(nameAnsi);
                        Memory.LocalFree(valueAnsi);
                        return attr;
                }
                public Haggle.Attribute.AttributeList GetAttributeList()
                {
                        if (UnmanagedGetAttributeList(cDataObject) == IntPtr.Zero)
                                return null;
                        return Haggle.Attribute.AttributeList.Copy(UnmanagedGetAttributeList(cDataObject));
                }
                public string GetFileName()
                {
                        if (cDataObject == IntPtr.Zero)
                        {
                                Debug.WriteLine("Data Object pointer is null");
                                return "";
                        }
                        IntPtr fn = UnmanagedGetFileName(this.cDataObject);

                        if (fn == IntPtr.Zero)
                                return "";

                        return Utility.AnsiIntPtrToString(fn);
                }

                public string GetFilePath()
                {
                        if (cDataObject == IntPtr.Zero)
                        {
                                Debug.WriteLine("Data Object pointer is null");
                                return "";
                        }

                        IntPtr fp = UnmanagedGetFilePath(this.cDataObject);

                        if (fp == IntPtr.Zero)
                                return "";

                        return Utility.AnsiIntPtrToString(fp) ;
                }

                public ulong GetNumAttributes()
                {
                        return UnmanagedGetNumAttributes(this.cDataObject);
                }

                #region IDisposable Members

                public void Dispose()
                {
                        if (this.cDataObject != IntPtr.Zero)
                        {
                                Debug.WriteLine("Disposing data object " + num);
                                UnmanagedFree(this.cDataObject);
                                this.cDataObject = IntPtr.Zero;
                        }

                        GC.SuppressFinalize(this);
                }

                #endregion
        }
}
