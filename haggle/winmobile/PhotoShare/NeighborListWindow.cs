using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using Haggle;

namespace PhotoShare
{
        public partial class NeighborListWindow : Form
        {
                public object neighborListLock = new object();

                public NeighborListWindow()
                {
                        InitializeComponent();
                }
                public void doNeighborListUpdate(Node.NodeList neighborList)
                {
                        Debug.WriteLine("doNeighborListUpdate");

                        lock (neighborListLock)
                        {
                                neighborListView.Items.Clear();

                                neighborListView.BeginUpdate();

                                Debug.WriteLine("Looping through neighbors");
                                foreach (Haggle.Node neighbor in neighborList.AsArray())
                                {
                                        string ifaceStr = "";
                                        Debug.WriteLine("Adding neighbor " + neighbor.GetName());
                                        ListViewItem neighItem = new ListViewItem(neighbor.GetName());
                                        foreach (Node.Interface iface in neighbor.InterfacesArray())
                                        {
                                                ifaceStr += iface.GetIdentifierStr() + ",";
                                        }

                                        char[] tc = { ',' };
                                        neighItem.SubItems.Add(ifaceStr.TrimEnd(tc));
                                        neighborListView.Items.Add(neighItem);

                                }
                                neighborListView.EndUpdate();
                                Debug.WriteLine("Neighborlist update end");
                        }
                }

                private void menuItemBack_Click(object sender, EventArgs e)
                {
                        this.DialogResult = DialogResult.OK;
                        this.Close();
                }

                private void NeighborListWindow_KeyDown(object sender, KeyEventArgs e)
                {
                    if ((e.KeyCode == System.Windows.Forms.Keys.Up))
                    {
                        // Up
                    }
                    if ((e.KeyCode == System.Windows.Forms.Keys.Down))
                    {
                        // Down
                    }
                    if ((e.KeyCode == System.Windows.Forms.Keys.Left))
                    {
                        // Left
                    }
                    if ((e.KeyCode == System.Windows.Forms.Keys.Right))
                    {
                        // Right
                    }
                    if ((e.KeyCode == System.Windows.Forms.Keys.Enter))
                    {
                        // Enter
                    }

                }

                private void neighborListView_SelectedIndexChanged(object sender, EventArgs e)
                {

                }
        }
}