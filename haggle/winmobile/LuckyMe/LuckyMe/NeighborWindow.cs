using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace LuckyGUI
{
        public partial class NeighborWindow : Form
        {
                public NeighborWindow()
                {
                        InitializeComponent();
                }

                private void menuItem2_Click(object sender, EventArgs e)
                {
                        this.DialogResult = DialogResult.OK;
                        this.Close();
                }

                public void onNeighborUpdate()
                {
                        // clear list:
                        listView1.Items.Clear();
                        // fill in list!
                        uint i, j;

                        LuckyMeLib.neighborListLock();

                        j = LuckyMeLib.getNumberOfNeighbors();

                        for (i = 0; i < j; i++)
                        {
                                listView1.Items.Add(new ListViewItem(LuckyMeLib.getNeighborName(i)));
                        }

                        LuckyMeLib.neighborListUnlock();
                }
        }
}