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
namespace PhotoShare
{
        partial class FileAttributeWindow
        {
                /// <summary>
                /// Required designer variable.
                /// </summary>
                private System.ComponentModel.IContainer components = null;
                private System.Windows.Forms.MainMenu mainMenu1;

                /// <summary>
                /// Clean up any resources being used.
                /// </summary>
                /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
                protected override void Dispose(bool disposing)
                {
                        if (disposing && (components != null))
                        {
                                components.Dispose();
                        }
                        base.Dispose(disposing);
                }

                #region Windows Form Designer generated code

                /// <summary>
                /// Required method for Designer support - do not modify
                /// the contents of this method with the code editor.
                /// </summary>
                private void InitializeComponent()
                {
                        this.mainMenu1 = new System.Windows.Forms.MainMenu();
                        this.menuItem1 = new System.Windows.Forms.MenuItem();
                        this.menuItem2 = new System.Windows.Forms.MenuItem();
                        this.textBox1 = new System.Windows.Forms.TextBox();
                        this.label1 = new System.Windows.Forms.Label();
                        this.pictureBox1 = new System.Windows.Forms.PictureBox();
                        this.label3 = new System.Windows.Forms.Label();
                        this.checkBoxAddAsInterest = new System.Windows.Forms.CheckBox();
                        this.SuspendLayout();
                        // 
                        // mainMenu1
                        // 
                        this.mainMenu1.MenuItems.Add(this.menuItem1);
                        this.mainMenu1.MenuItems.Add(this.menuItem2);
                        // 
                        // menuItem1
                        // 
                        this.menuItem1.Text = "Cancel";
                        this.menuItem1.Click += new System.EventHandler(this.menuItem1_Click);
                        // 
                        // menuItem2
                        // 
                        this.menuItem2.Text = "Done";
                        this.menuItem2.Click += new System.EventHandler(this.menuItem2_Click);
                        // 
                        // textBox1
                        // 
                        this.textBox1.Font = new System.Drawing.Font("Tahoma", 10F, System.Drawing.FontStyle.Regular);
                        this.textBox1.Location = new System.Drawing.Point(16, 28);
                        this.textBox1.Name = "textBox1";
                        this.textBox1.Size = new System.Drawing.Size(208, 23);
                        this.textBox1.TabIndex = 0;
                        this.textBox1.TextChanged += new System.EventHandler(this.textBox1_TextChanged);
                        this.textBox1.KeyDown += new System.Windows.Forms.KeyEventHandler(this.textBox1_KeyDown);
                        // 
                        // label1
                        // 
                        this.label1.Font = new System.Drawing.Font("Tahoma", 10F, System.Drawing.FontStyle.Bold);
                        this.label1.Location = new System.Drawing.Point(16, 4);
                        this.label1.Name = "label1";
                        this.label1.Size = new System.Drawing.Size(98, 23);
                        this.label1.Text = "Attribute(s):";
                        this.label1.ParentChanged += new System.EventHandler(this.label1_ParentChanged);
                        // 
                        // pictureBox1
                        // 
                        this.pictureBox1.Location = new System.Drawing.Point(16, 86);
                        this.pictureBox1.Name = "pictureBox1";
                        this.pictureBox1.Size = new System.Drawing.Size(208, 177);
                        // 
                        // label3
                        // 
                        this.label3.Font = new System.Drawing.Font("Tahoma", 12F, System.Drawing.FontStyle.Regular);
                        this.label3.Location = new System.Drawing.Point(16, 59);
                        this.label3.Name = "label3";
                        this.label3.Size = new System.Drawing.Size(208, 26);
                        this.label3.Text = "fileNameLabel";
                        this.label3.ParentChanged += new System.EventHandler(this.label3_ParentChanged);
                        // 
                        // checkBoxAddAsInterest
                        // 
                        this.checkBoxAddAsInterest.Checked = true;
                        this.checkBoxAddAsInterest.CheckState = System.Windows.Forms.CheckState.Checked;
                        this.checkBoxAddAsInterest.Location = new System.Drawing.Point(106, 0);
                        this.checkBoxAddAsInterest.Name = "checkBoxAddAsInterest";
                        this.checkBoxAddAsInterest.Size = new System.Drawing.Size(130, 27);
                        this.checkBoxAddAsInterest.TabIndex = 3;
                        this.checkBoxAddAsInterest.Text = "Add as interest(s)";
                        // 
                        // FileAttributeWindow
                        // 
                        this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
                        this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
                        this.AutoScroll = true;
                        this.ClientSize = new System.Drawing.Size(238, 266);
                        this.Controls.Add(this.checkBoxAddAsInterest);
                        this.Controls.Add(this.label3);
                        this.Controls.Add(this.pictureBox1);
                        this.Controls.Add(this.label1);
                        this.Controls.Add(this.textBox1);
                        this.Menu = this.mainMenu1;
                        this.Name = "FileAttributeWindow";
                        this.Text = "PhotoShare: Add Attributes";
                        this.Load += new System.EventHandler(this.FileAttributeWindow_Load);
                        this.ResumeLayout(false);

                }

                #endregion

                private System.Windows.Forms.TextBox textBox1;
                private System.Windows.Forms.Label label1;
                private System.Windows.Forms.MenuItem menuItem1;
                private System.Windows.Forms.PictureBox pictureBox1;
                private System.Windows.Forms.Label label3;
                private System.Windows.Forms.MenuItem menuItem2;
                private System.Windows.Forms.CheckBox checkBoxAddAsInterest;
        }
}
