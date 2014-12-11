using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;

namespace PhotoShare
{
        public partial class AddInterestWindow : Form
        {
                Haggle.Attribute.AttributeList addInterestList = new Haggle.Attribute.AttributeList();
                Haggle.Attribute.AttributeList delInterestList = new Haggle.Attribute.AttributeList();

                public AddInterestWindow()
                {
                        Debug.WriteLine("Constructor interest lists view");
                        InitializeComponent();

                        interestListView.Columns.Add("Column", interestListView.Width, HorizontalAlignment.Center);
                }
                public Haggle.Attribute.AttributeList getAddInterests()
                {
                        return addInterestList;
                }
                public Haggle.Attribute.AttributeList getDelInterests()
                {
                        return delInterestList;
                }
                private void ParseInterestTextBox()
                {
                        // 1. Get the input text in texbox1 and add to interest.
                        // 2. Update listview
                        string[] strArray = interestTextBox.Text.Split(',');
                        interestTextBox.Text = "";
                        int num = 0;

                        if (strArray.Length == 0)
                        {
                                return;
                        }
                        interestListView.BeginUpdate();

                        foreach (string str in strArray)
                        {
                                bool canAdd = true;
                                string trimstr = str.TrimEnd();

                                if (trimstr.Length == 0)
                                        continue;

                                // Check that this attribute doesn't already exist:
                                for (int i = 0; i < interestListView.Items.Count && canAdd; i++)
                                {
                                    string attrvalue = interestListView.Items[i].Text;
                                    string[] avw = attrvalue.Split(':');

                                    if (Equals(avw[0], trimstr))
                                        canAdd = false;
                                }

                                if (canAdd)
                                {
                                    Haggle.Attribute a = new Haggle.Attribute("Picture", trimstr, 1);

                                    if (!delInterestList.Contains(a))
                                    {
                                        // Add weight to string
                                            
                                        trimstr += ":" + 1;
                                        //trimstr += ":" + trackBarWeight.Value;

                                        ListViewItem lvi = new ListViewItem(trimstr);
                                        interestListView.Items.Add(lvi);
                                        addInterestList.Add(a);
                                        delInterestList.Remove(a);
                                        num++;
                                    }
                                }
                        }

                        interestListView.EndUpdate();

                        if (num > 0)
                                menuItem1.Text = "Done";
                }

                public long interestListUpdate(Haggle.Attribute.AttributeList al)
                {
                    long numberOfAttributesAdded = 0;

                    Haggle.Attribute[] attrs = al.AsArray();

                    interestListView.BeginUpdate();

                    foreach (Haggle.Attribute a in attrs)
                    {
                        bool canAdd = true;

                        if (a.GetName() != "Picture")
                            continue;

                        string str = a.GetValue();

                        // Check that this attribute doesn't already exist:
                        for (int i = 0; i < interestListView.Items.Count && canAdd; i++)
                        {
                            string attrvalue = interestListView.Items[i].Text;
                            string[] avw = attrvalue.Split(':');

                            if (Equals(avw[0], str))
                                canAdd = false;
                        }

                        if (canAdd)
                        {
                            // Add weight to string
                            str += ":" + a.GetWeight();

                            ListViewItem lvi = new ListViewItem(str);
                            interestListView.Items.Add(lvi);
                            numberOfAttributesAdded++;
                        }
                    }

                    interestListView.EndUpdate();

                    return numberOfAttributesAdded;
                }
                private void menuItem2_Click(object sender, EventArgs e)
                {
                        if (interestListView.Focused)
                        {
                                // Remove interest attribute
                                for (int i = 0; i < interestListView.SelectedIndices.Count; i++)
                                {
                                        int index = interestListView.SelectedIndices[i];
                                        string attrvalue = interestListView.Items[index].Text;
                                        Debug.WriteLine("remove : " + attrvalue + " at index " + index);

                                        // Separate value and weight
                                        string[] avw = attrvalue.Split(':');

                                        Haggle.Attribute a = new Haggle.Attribute("Picture", avw[0]);

                                        if (!delInterestList.Contains(a))
                                        {
                                            if (addInterestList.Contains(a))
                                            {
                                                addInterestList.Remove(a);
                                            }
                                            else
                                            {
                                                delInterestList.Add(a);
                                            }
                                            interestListView.Items.RemoveAt(index);
                                            menuItem1.Text = "Done";
                                        }
                                }
                        } else if (interestTextBox.Text.Length > 0)
                        {
                                ParseInterestTextBox();
                        }
                }

                private void menuItem1_Click(object sender, EventArgs e)
                {
                        if (delInterestList.Size() > 0 || addInterestList.Size() > 0)
                                 this.DialogResult = DialogResult.OK;
                        else
                                 this.DialogResult = DialogResult.Cancel;

                        menuItem1.Text = "Cancel";
                        this.Close();
                }

                private void interestListView_SelectedIndexChanged(object sender, EventArgs e)
                {
                        menuItem2.Text = "Delete";
                }

                private void AddInterestWindow_Load(object sender, EventArgs e)
                {
                        Debug.WriteLine("Clearing interest lists");
                        addInterestList.Clear();
                        delInterestList.Clear();
                }

                private void interestTextBox_TextChanged(object sender, EventArgs e)
                {
                       
                }

                private void interestBox_KeyDown(object sender, KeyEventArgs e)
                {
                        if (e.KeyCode == Keys.Enter)
                        {
                                ParseInterestTextBox();
                        }
                }

                private void interestListView_KeyPress(object sender, KeyPressEventArgs e)
                {

                }

                private void interestTextBox_GotFocus(object sender, EventArgs e)
                {
                        menuItem2.Text = "Add";
                }

                private void interestListView_GotFocus(object sender, EventArgs e)
                {
                        menuItem2.Text = "Delete";
                }

                private void AddInterestWindow_KeyDown(object sender, KeyEventArgs e)
                {
                        if ((e.KeyCode == System.Windows.Forms.Keys.F1))
                        {
                                // Soft Key 1
                                // Not handled when menu is present.
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.F2))
                        {
                                // Soft Key 2
                                // Not handled when menu is present.
                        }
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
                        if ((e.KeyCode == System.Windows.Forms.Keys.D1))
                        {
                                // 1
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.D2))
                        {
                                // 2
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.D3))
                        {
                                // 3
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.D4))
                        {
                                // 4
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.D5))
                        {
                                // 5
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.D6))
                        {
                                // 6
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.D7))
                        {
                                // 7
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.D8))
                        {
                                // 8
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.D9))
                        {
                                // 9
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.F8))
                        {
                                // *
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.D0))
                        {
                                // 0
                        }
                        if ((e.KeyCode == System.Windows.Forms.Keys.F9))
                        {
                                // #
                        }

                }

                private void label1_ParentChanged(object sender, EventArgs e)
                {

                }
        }
}