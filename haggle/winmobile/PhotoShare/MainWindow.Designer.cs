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
    partial class MainWindow
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
                this.menuEditInterests = new System.Windows.Forms.MenuItem();
                this.menuNeighbors = new System.Windows.Forms.MenuItem();
                this.menuItem2 = new System.Windows.Forms.MenuItem();
                this.menuQuit = new System.Windows.Forms.MenuItem();
                this.menuShutdown = new System.Windows.Forms.MenuItem();
                this.menuTakePicture = new System.Windows.Forms.MenuItem();
                this.photoListView = new System.Windows.Forms.ListView();
                this.photoImageList = new System.Windows.Forms.ImageList();
                this.myPicturesLabel = new System.Windows.Forms.Label();
                this.SuspendLayout();
                // 
                // mainMenu1
                // 
                this.mainMenu1.MenuItems.Add(this.menuItem1);
                this.mainMenu1.MenuItems.Add(this.menuTakePicture);
                // 
                // menuItem1
                // 
                this.menuItem1.MenuItems.Add(this.menuEditInterests);
                this.menuItem1.MenuItems.Add(this.menuNeighbors);
                this.menuItem1.MenuItems.Add(this.menuItem2);
                this.menuItem1.MenuItems.Add(this.menuQuit);
                this.menuItem1.MenuItems.Add(this.menuShutdown);
                this.menuItem1.Text = "Menu";
                this.menuItem1.Click += new System.EventHandler(this.menuItem1_Click);
                // 
                // menuEditInterests
                // 
                this.menuEditInterests.Text = "Edit Interests";
                this.menuEditInterests.Click += new System.EventHandler(this.menuAddInterest_Click);
                // 
                // menuNeighbors
                // 
                this.menuNeighbors.Text = "Neighbors";
                this.menuNeighbors.Click += new System.EventHandler(this.menuItemViewNeighbors_Click);
                // 
                // menuItem2
                // 
                this.menuItem2.Text = "-";
                // 
                // menuQuit
                // 
                this.menuQuit.Text = "Quit";
                this.menuQuit.Click += new System.EventHandler(this.menuQuit_Click);
                // 
                // menuShutdown
                // 
                this.menuShutdown.Text = "Shutdown Haggle";
                this.menuShutdown.Click += new System.EventHandler(this.shutdownHaggle_Click);
                // 
                // menuTakePicture
                // 
                this.menuTakePicture.Text = "Take Picture";
                this.menuTakePicture.Click += new System.EventHandler(this.menuTakePicture_Click);
                // 
                // photoListView
                // 
                this.photoListView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom)
                            | System.Windows.Forms.AnchorStyles.Left)
                            | System.Windows.Forms.AnchorStyles.Right)));
                this.photoListView.BackColor = System.Drawing.Color.WhiteSmoke;
                this.photoListView.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
                this.photoListView.LargeImageList = this.photoImageList;
                this.photoListView.Location = new System.Drawing.Point(0, 24);
                this.photoListView.Name = "photoListView";
                this.photoListView.Size = new System.Drawing.Size(239, 245);
                this.photoListView.TabIndex = 1;
                this.photoListView.SelectedIndexChanged += new System.EventHandler(this.photoListView_SelectedIndexChanged);
                // 
                // photoImageList
                // 
                this.photoImageList.ImageSize = new System.Drawing.Size(128, 128);
                // 
                // myPicturesLabel
                // 
                this.myPicturesLabel.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Left)
                            | System.Windows.Forms.AnchorStyles.Right)));
                this.myPicturesLabel.BackColor = System.Drawing.Color.Green;
                this.myPicturesLabel.Font = new System.Drawing.Font("Tahoma", 14F, System.Drawing.FontStyle.Regular);
                this.myPicturesLabel.ForeColor = System.Drawing.SystemColors.ActiveCaptionText;
                this.myPicturesLabel.Location = new System.Drawing.Point(0, 0);
                this.myPicturesLabel.Name = "myPicturesLabel";
                this.myPicturesLabel.Size = new System.Drawing.Size(239, 26);
                this.myPicturesLabel.Text = "My Photos";
                this.myPicturesLabel.TextAlign = System.Drawing.ContentAlignment.TopCenter;
                this.myPicturesLabel.ParentChanged += new System.EventHandler(this.label1_ParentChanged);
                // 
                // MainWindow
                // 
                this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
                this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
                this.AutoScroll = true;
                this.ClientSize = new System.Drawing.Size(239, 269);
                this.Controls.Add(this.myPicturesLabel);
                this.Controls.Add(this.photoListView);
                this.KeyPreview = true;
                this.Menu = this.mainMenu1;
                this.Name = "MainWindow";
                this.Text = "PhotoShare";
                this.Load += new System.EventHandler(this.PhotoShareForm_Load);
                this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.MenuItem menuItem1;
        private System.Windows.Forms.MenuItem menuItem2;
        private System.Windows.Forms.MenuItem menuQuit;
        private System.Windows.Forms.MenuItem menuTakePicture;
        public System.Windows.Forms.ListView photoListView;
        private System.Windows.Forms.MenuItem menuShutdown;
        private System.Windows.Forms.MenuItem menuEditInterests;
        private System.Windows.Forms.MenuItem menuNeighbors;
        public System.Windows.Forms.ImageList photoImageList;
        private System.Windows.Forms.Label myPicturesLabel;
    }
}

