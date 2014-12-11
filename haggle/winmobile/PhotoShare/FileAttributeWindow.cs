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
        public partial class FileAttributeWindow : Form
        {
                string resultValue = "";
                bool addAsInterest = true;
                string filename;
                
                public FileAttributeWindow(string inFilename)
                {
                        InitializeComponent();
                        filename = inFilename.Substring(inFilename.LastIndexOf("\\") + 1);
                        Debug.WriteLine("filename is " + inFilename);
                        pictureBox1.Image = new Bitmap(inFilename);
                        pictureBox1.SizeMode = PictureBoxSizeMode.StretchImage;
                        label3.Text = filename;
                }
                
                private void label1_ParentChanged(object sender, EventArgs e)
                {

                }

                private void textBox1_TextChanged(object sender, EventArgs e)
                {

                }

                private void menuItem1_Click(object sender, EventArgs e)
                {
                        this.DialogResult = DialogResult.Cancel;
                        this.Close();
                }

                public string getResultValueString() 
                {
                        return resultValue;
                }
                public bool getAddAsInterest()
                {
                        return addAsInterest;
                }
                private void label3_ParentChanged(object sender, EventArgs e)
                {

                }

                private void pictureBox1_Click(object sender, EventArgs e)
                {

                }

                private void menuItem2_Click(object sender, EventArgs e)
                {
                        resultValue = textBox1.Text.TrimEnd();

                        if (resultValue.Length > 0)
                        {
                                this.DialogResult = DialogResult.OK;
                                this.addAsInterest = checkBoxAddAsInterest.Checked;
                                this.Close();
                        }
                }

                private void FileAttributeWindow_Load(object sender, EventArgs e)
                {

                }

                private void textBox1_KeyDown(object sender, KeyEventArgs e)
                {
                        if (e.KeyCode == Keys.Enter)
                        {
                                resultValue = textBox1.Text.TrimEnd();

                                if (resultValue.Length > 0)
                                {
                                        this.DialogResult = DialogResult.OK;
                                        this.Close();
                                }
                        }
                }
        }
}
