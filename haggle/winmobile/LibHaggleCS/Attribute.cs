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
using System.Diagnostics;

namespace Haggle
{
        public class Attribute
        {
                string name;
                string value;
                UInt32 weight;

                [DllImport("libhaggle.dll", EntryPoint = "haggle_attribute_new")]
                static extern IntPtr UnmanagedNew(IntPtr name, IntPtr value);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_attribute_new_weighted")]
                static extern IntPtr UnmanagedNew(IntPtr name, IntPtr value, UInt32 weight);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_attribute_free")]
                static extern void UnmanagedFree(IntPtr a);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_attribute_get_name")]
                static extern IntPtr UnmanagedGetName(IntPtr a);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_attribute_get_value")]
                static extern IntPtr UnmanagedGetValue(IntPtr a);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_attribute_get_weight")]
                static extern UInt32 UnmanagedGetWeight(IntPtr a);


                public string GetName()
                {
                        return name;
                }
                public string GetValue()
                {
                        return value;
                }
                public UInt32 GetWeight()
                {
                        return weight;
                }
                public class AttributeNullPtrException : NullReferenceException { }
                public class AttributeException : Exception
                {
                        string msg;

                        public AttributeException(string _msg)
                        {
                                msg = _msg;
                        }
                        public override string ToString()
                        {
                                return msg;
                        }
                }
                public override string ToString() 
                {
                       string ret = name;

                       return (ret + "=" + value + ":" + weight);
                }
                public Attribute(IntPtr cAttr, bool freeIt)
                {
                        if (cAttr == IntPtr.Zero)
                        {
                                throw new AttributeNullPtrException();
                        } 
                        name = Utility.AnsiIntPtrToString(UnmanagedGetName(cAttr));
                        value = Utility.AnsiIntPtrToString(UnmanagedGetValue(cAttr));
                        weight = UnmanagedGetWeight(cAttr);

                        if (freeIt)
                                UnmanagedFree(cAttr);
                }
                public Attribute(string name, string value)
                {  
                        /*
                        IntPtr nameAnsi = Utility.StringToAnsiIntPtr(name);
                        IntPtr valueAnsi = Utility.StringToAnsiIntPtr(value);
                        
                        cAttribute = UnmanagedNew(nameAnsi, valueAnsi);

                        Memory.LocalFree(nameAnsi);
                        Memory.LocalFree(valueAnsi);
                        
                        if (cAttribute == IntPtr.Zero)
                        {
                                throw new AttributeNullPtrException();
                        }
                        */
                        this.name = name;
                        this.value = value;
                        this.weight = 1;
                }
                public Attribute(string name, string value, UInt32 weight)
                {
                        /*
                        IntPtr nameAnsi = Utility.StringToAnsiIntPtr(name);
                        IntPtr valueAnsi = Utility.StringToAnsiIntPtr(value);

                        cAttribute = UnmanagedNew(nameAnsi, valueAnsi, weight);

                        Memory.LocalFree(nameAnsi);
                        Memory.LocalFree(valueAnsi);
                        
                        if (cAttribute == IntPtr.Zero)
                                throw new AttributeNullPtrException();
                        */
                        this.name = name;
                        this.value = value;
                        this.weight = weight;

                        Debug.WriteLine("New attribute: " + this.ToString());
                }
                ~Attribute()
                {
                        Debug.WriteLine("Attribute destructor");
                }
                /*
                protected virtual void Dispose(bool disposing)
                {
                        // Check to see if Dispose has already been called.
                        if (!this.disposed)
                        {
                                // If disposing equals true, dispose all managed 
                                // and unmanaged resources.
                                if (disposing)
                                {
                                       // Dispose managed resources.
                                }
                                if (!isPartOfList)
                                {
                                        UnmanagedFree(cAttribute);
                                        Debug.WriteLine("Attribute " + this.ToString() + " disposed");
                                }
                        }
                        disposed = true;
                }

                #region IDisposable Members

                public void Dispose()
                {
                        Debug.WriteLine("Disposing of Attribute " + this.ToString());
                        Dispose(true);
                        GC.SuppressFinalize(this);
                }

                #endregion
                 */
                public class AttributeList : IDisposable
                {
                        public IntPtr cAttrList;
                        private bool disposed = false;

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_attributelist_new")]
                        static extern IntPtr UnmanagedNew();

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_attributelist_new_from_attribute")]
                        static extern IntPtr UnmanagedNewFromAttribute(IntPtr al);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_attributelist_copy")]
                        static extern IntPtr UnmanagedCopy(IntPtr al);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_attributelist_free")]
                        static extern void UnmanagedFree(IntPtr al);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_attributelist_add_attribute")]
                        static extern UInt32 UnmanagedAddAttribute(IntPtr al, IntPtr a);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_attributelist_remove_attribute")]
                        static extern IntPtr UnmanagedRemoveAttribute(IntPtr al, IntPtr name, IntPtr value);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_attributelist_get_attribute_n")]
                        static extern IntPtr UnmanagedGetAttribute(IntPtr al, UInt32 n);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_attributelist_get_attribute_by_name")]
                        static extern IntPtr UnmanagedGetAttribute(IntPtr al, IntPtr name);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_attributelist_get_attribute_by_name_n")]
                        static extern IntPtr UnmanagedGetAttribute(IntPtr al, IntPtr name, UInt32 n);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_attributelist_get_attribute_by_name_value")]
                        static extern IntPtr UnmanagedGetAttribute(IntPtr al, IntPtr name, IntPtr value);
                        
                        [DllImport("libhaggle.dll", EntryPoint = "haggle_attributelist_detach_attribute")]
                        static extern IntPtr UnmanagedDetachAttribute(IntPtr al, IntPtr a);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_attributelist_size")]
                        static extern UInt32 UnmanagedSize(IntPtr al);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_attributelist_pop")]
                        static extern IntPtr UnmanagedPop(IntPtr al);

                        public AttributeList()
                        {
                                cAttrList = UnmanagedNew();

                                if (cAttrList == IntPtr.Zero)
                                {
                                        throw new AttributeNullPtrException();
                                }
                        }
                        public AttributeList(IntPtr al)
                        {
                                cAttrList = al;

                                if (cAttrList == IntPtr.Zero)
                                {
                                        throw new AttributeNullPtrException();
                                }
                        }
                        public AttributeList(Attribute a)
                        {
                                IntPtr nameAnsi = Utility.StringToAnsiIntPtr(a.GetName());
                                IntPtr valueAnsi = Utility.StringToAnsiIntPtr(a.GetValue());

                                cAttrList = UnmanagedNewFromAttribute(Attribute.UnmanagedNew(nameAnsi, valueAnsi, a.GetWeight()));

                                Memory.LocalFree(nameAnsi);
                                Memory.LocalFree(valueAnsi);

                                if (cAttrList == IntPtr.Zero)
                                {
                                        throw new AttributeNullPtrException();
                                }
                        }
                        ~AttributeList()
                        {
                                Debug.WriteLine("AttributeList destructor");
                                Dispose(false);

                                GC.SuppressFinalize(this);
                        }
                        public static AttributeList Copy(IntPtr al)
                        {
                                return new AttributeList(UnmanagedCopy(al));
                        }
                        public ulong Add(Attribute a)
                        {
                                Debug.WriteLine("Adding new attribute:" + a.ToString());

                                IntPtr nameAnsi = Utility.StringToAnsiIntPtr(a.GetName());
                                IntPtr valueAnsi = Utility.StringToAnsiIntPtr(a.GetValue());

                                UInt32 size = UnmanagedAddAttribute(cAttrList, Attribute.UnmanagedNew(nameAnsi, valueAnsi, a.GetWeight()));

                                Memory.LocalFree(nameAnsi);
                                Memory.LocalFree(valueAnsi);

                                Debug.WriteLine("Added attribute " + a.ToString() + ". New size=" + size);

                                return size;
                        }
                        public Attribute Get(string name, string value)
                        {
                                IntPtr nameAnsi = Utility.StringToAnsiIntPtr(name);
                                IntPtr valueAnsi = Utility.StringToAnsiIntPtr(value);

                                IntPtr cAttr = UnmanagedGetAttribute(cAttrList, nameAnsi, valueAnsi);
                                        
                                Memory.LocalFree(nameAnsi);
                                Memory.LocalFree(valueAnsi);

                                if (cAttr == IntPtr.Zero)
                                        throw new AttributeNullPtrException();

                                return new Attribute(cAttr, false);
                        }
                        public Attribute Get(string name, UInt32 n)
                        {
                                IntPtr nameAnsi = Utility.StringToAnsiIntPtr(name);

                                IntPtr cAttr = UnmanagedGetAttribute(cAttrList, nameAnsi, n);

                                Memory.LocalFree(nameAnsi);

                                if (cAttr == IntPtr.Zero)
                                        throw new AttributeNullPtrException();

                                return new Attribute(cAttr, false);
                        }
                        public Attribute Get(UInt32 n)
                        {
                                IntPtr cAttr = UnmanagedGetAttribute(cAttrList, n);

                                if (cAttr == IntPtr.Zero)
                                        throw new AttributeNullPtrException();

                                return new Attribute(cAttr, false);
                        }
                        public bool Contains(Attribute a)
                        {
                                try
                                {
                                        Get(a.name, a.value);
                                }
                                catch (AttributeNullPtrException)
                                {
                                        return false;
                                }
                                return true;
                        }
                        public ulong Size()
                        {
                                return UnmanagedSize(cAttrList);
                        }
                        public Attribute Pop()
                        {
                               return new Attribute(UnmanagedPop(cAttrList), true);
                        }
                        public void Clear()
                        {
                                UnmanagedFree(cAttrList);

                                cAttrList = UnmanagedNew();

                                if (cAttrList == IntPtr.Zero)
                                {
                                        throw new AttributeNullPtrException();
                                }
                        }
                        public bool Remove(string name, string value)
                        {
                                IntPtr cAttr;
                                IntPtr nameAnsi = Utility.StringToAnsiIntPtr(name);
                                IntPtr valueAnsi = Utility.StringToAnsiIntPtr(value);

                                cAttr = UnmanagedRemoveAttribute(cAttrList, nameAnsi, valueAnsi);

                                if (cAttr == IntPtr.Zero)
                                        return false;

                                Memory.LocalFree(nameAnsi);
                                Memory.LocalFree(valueAnsi);

                                Attribute.UnmanagedFree(cAttr);

                                return true;
                        }
                        public bool Remove(Attribute a)
                        {
                                return Remove(a.name, a.value);
                        }
                        public Attribute[] AsArray()
                        {
                                System.Collections.ArrayList arrList = new System.Collections.ArrayList();
                                UInt32 i = 0;

                                while (true)
                                {
                                       IntPtr cAttr;
                                       Attribute a;

                                        cAttr = UnmanagedGetAttribute(cAttrList, i);

                                        if (cAttr == IntPtr.Zero)
                                        {
                                                //Debug.WriteLine("UnmanagedAttribute zero pointer");
                                                break;
                                        }

                                        a = new Attribute(cAttr, false);
                                        i++;
                                        arrList.Add(a);
                                }
                                Attribute[] array = (Attribute[])arrList.ToArray(typeof(Attribute));

                                Debug.WriteLine("AttrList with " + i + " attributes");

                                return array;
                        }
                        protected virtual void Dispose(bool disposing)
                        {
                                // Check to see if Dispose has already been called.
                                if (!this.disposed)
                                {
                                        // If disposing equals true, dispose all managed 
                                        // and unmanaged resources.
                                        if (disposing)
                                        {
                                                // Dispose managed resources.
                                        }

                                        UnmanagedFree(this.cAttrList);
                                        this.cAttrList = IntPtr.Zero;
                                        Debug.WriteLine("AttributeList disposed");
                                }
                                disposed = true;
                        }

                        #region IDisposable Members

                        public void Dispose()
                        {
                                Debug.WriteLine("Disposing of AttributeList");
                                
                                Dispose(true);

                                GC.SuppressFinalize(this);
                        }

                        #endregion
                }
        }
}
