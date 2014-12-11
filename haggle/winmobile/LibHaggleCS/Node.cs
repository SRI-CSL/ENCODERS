using System;
using System.Linq;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Diagnostics;

namespace Haggle
{     
        [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Auto)]
        public class Node : IDisposable
        {
                public IntPtr cNode;
                string name;
                public bool isPartOfList;
                private bool disposed = false;

                [DllImport("libhaggle.dll", EntryPoint = "haggle_node_free")]
                static extern void UnmanagedFree(IntPtr node);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_node_get_name")]
                static extern IntPtr UnmanagedGetName(IntPtr node);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_node_get_num_interfaces")]
                static extern int UnmanagedGetNumInterfaces(IntPtr node);

                [DllImport("libhaggle.dll", EntryPoint = "haggle_node_get_interface_n")]
                static extern IntPtr UnmanagedGetInterfaceN(IntPtr node, int n);
                
                private Node(IntPtr _cNode, bool isPartOfList) 
                {
                        this.isPartOfList = isPartOfList;
                        cNode = _cNode;
                        name = Utility.AnsiIntPtrToString(UnmanagedGetName(cNode));
                }
                ~Node()
                {
                        // Nodes only exists as part of NodeLists and therefore
                        // the unmanaged C-resource will be release when the list
                        // is deleted, unless the node has been popped from the list
                        Dispose(false);
                }
                public string GetName()
                {
                        return name;
                }
                public Interface GetInterfaceN(int n)
                {
                        return new Interface(UnmanagedGetInterfaceN(cNode, n));
                }
                public Interface[] InterfacesArray()
                {
                        System.Collections.ArrayList arrList = new System.Collections.ArrayList();
                        int i = 0;

                        while (true)
                        {
                                IntPtr cIf;
                                Interface iface;

                                cIf = UnmanagedGetInterfaceN(cNode, i);

                                if (cIf == IntPtr.Zero)
                                {
                                        //Debug.WriteLine("UnmanagedAttribute zero pointer");
                                        break;
                                }

                                iface = new Interface(cIf);
                                i++;
                                arrList.Add(iface);
                        }
                        Interface[] array = (Interface[])arrList.ToArray(typeof(Interface));

                        Debug.WriteLine("NodeList with " + i + " nodes");

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
                                if (!isPartOfList)
                                {
                                        UnmanagedFree(this.cNode);
                                        this.cNode = IntPtr.Zero;
                                        Debug.WriteLine("Node " + this.GetName() + " disposed");
                                }
                        }
                        disposed = true;
                }

                public void Dispose()
                {
                        Dispose(true);
                        GC.SuppressFinalize(this);
                }
                public class NodeException : Exception
                {
                        string msg;

                        public NodeException(string _msg)
                        {
                                msg = _msg;
                        }
                        public override string ToString()
                        {
                                return msg;
                        }
                }
                public class Interface : IDisposable
                {
                        IntPtr cInterface;
                        int type;
                        string name;
                        string typename;
                        string identifierStr;

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_interface_free")]
                        static extern void UnmanagedFree(IntPtr iface);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_interface_get_name")]
                        static extern IntPtr UnmanagedGetName(IntPtr iface);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_interface_get_type")]
                        static extern short UnmanagedGetType(IntPtr iface);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_interface_get_type_name")]
                        static extern IntPtr UnmanagedGetTypeName(IntPtr iface);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_interface_get_identifier")]
                        static extern IntPtr UnmanagedGetIdentifier(IntPtr iface);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_interface_get_identifier_length")]
                        static extern int UnmanagedGetIdentifierLen(IntPtr iface);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_interface_get_identifier_str")]
                        static extern IntPtr UnmanagedGetIdentifierStr(IntPtr iface);
                        
                        public const int IFTYPE_UNDEF = 0;
                        public const int IFTYPE_APPLICATION_PORT = 1;
                        public const int IFTYPE_APPLICATION_LOCAL = 2;
                        public const int IFTYPE_ETHERNET = 3;
                        public const int IFTYPE_WIFI = 4;
                        public const int IFTYPE_BLUETOOTH = 5;
                        public const int IFTYPE_MEDIA = 6;
                        public const int _IFTYPE_MAX = 7;

                        public Interface(IntPtr _cInterface)
                        {
                                cInterface = _cInterface;
                                name = Utility.AnsiIntPtrToString(UnmanagedGetName(cInterface));
                                type = UnmanagedGetType(cInterface);
                                typename = Utility.AnsiIntPtrToString(UnmanagedGetTypeName(cInterface));
                                identifierStr = Utility.AnsiIntPtrToString(UnmanagedGetIdentifierStr(cInterface));
                        }
                        ~Interface()
                        {
                                // Interfaces should only exist as part of 
                                // Nodes, so deleting the node will automatically
                                // free the unmanaged resources.
                        }
                        public string GetName()
                        {
                                return name;
                        }
                        public string GetIdentifierStr()
                        {
                                return identifierStr;
                        }
                        public void Dispose()
                        {
                                // Interfaces should only exist as part of 
                                // Nodes, so deleting the node will automatically
                                // free the unmanaged resources.
                        }
                }
                public class NodeList : IDisposable
                {
                        public IntPtr cNodeList;
                        private bool disposed = false;

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_nodelist_free")]
                        public static extern void UnmanagedFree(IntPtr nodeList);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_nodelist_pop")]
                        public static extern IntPtr UnmanagedPop(IntPtr nodeList);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_nodelist_get_node_n")]
                        public static extern IntPtr UnmanagedGetNode(IntPtr nodeList, int n);

                        [DllImport("libhaggle.dll", EntryPoint = "haggle_nodelist_size")]
                        public static extern int UnmanagedSize(IntPtr nodeList);

                        public NodeList(IntPtr nl)
                        {
                                this.cNodeList = nl;

                                if (cNodeList == IntPtr.Zero)
                                        throw new NodeException("Could not create node list\n");
                        }
                        ~NodeList()
                        {
                                Dispose(false);
                        }
                        public Node Pop()
                        {
                                Node node = new Node(UnmanagedPop(cNodeList), false);
                                return node;
                        }
                        public Node NodeN(int n)
                        {
                                return new Node(UnmanagedGetNode(cNodeList, n), true);
                        }
                        public int Size()
                        {
                                return UnmanagedSize(cNodeList);
                        }
                        public Node[] AsArray()
                        {
                                System.Collections.ArrayList arrList = new System.Collections.ArrayList();
                                int i = 0;

                                while (true)
                                {
                                        IntPtr n;
                                        Node node;

                                        n = UnmanagedGetNode(cNodeList, i);

                                        if (n == IntPtr.Zero)
                                        {
                                                //Debug.WriteLine("UnmanagedAttribute zero pointer");
                                                break;
                                        }

                                        node = new Node(n, true);
                                        i++;
                                        arrList.Add(node);
                                }
                                Node[] array = (Node[])arrList.ToArray(typeof(Node));
                                
                                Debug.WriteLine("NodeList with " + i + " nodes");
                               
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
                                      
                                        UnmanagedFree(this.cNodeList);
                                        this.cNodeList = IntPtr.Zero;
                                        Debug.WriteLine("NodeList disposed");
                                }
                                disposed = true;
                        }
                        public void Dispose()
                        {
                                Dispose(true);
                                GC.SuppressFinalize(this);
                        }
                }
        }
}
