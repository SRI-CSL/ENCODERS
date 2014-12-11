namespace PhotoShare
{
        partial class AddInterestWindow
        {
                /// <summary>
                /// Required designer variable.
                /// </summary>
                private System.ComponentModel.IContainer components = null;

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
                    this.interestTextBox = new System.Windows.Forms.TextBox();
                    this.interestListView = new System.Windows.Forms.ListView();
                    this.InterestColumnHeader = new System.Windows.Forms.ColumnHeader();
                    this.label1 = new System.Windows.Forms.Label();
                    this.interestListLabel = new System.Windows.Forms.Label();
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
                    this.menuItem2.Text = "Add";
                    this.menuItem2.Click += new System.EventHandler(this.menuItem2_Click);
                    // 
                    // interestTextBox
                    // 
                    this.interestTextBox.BackColor = System.Drawing.Color.WhiteSmoke;
                    this.interestTextBox.Font = new System.Drawing.Font("Tahoma", 11F, System.Drawing.FontStyle.Regular);
                    this.interestTextBox.Location = new System.Drawing.Point(15, 25);
                    this.interestTextBox.Name = "interestTextBox";
                    this.interestTextBox.Size = new System.Drawing.Size(206, 24);
                    this.interestTextBox.TabIndex = 1;
                    this.interestTextBox.TextChanged += new System.EventHandler(this.interestTextBox_TextChanged);
                    this.interestTextBox.GotFocus += new System.EventHandler(this.interestTextBox_GotFocus);
                    this.interestTextBox.KeyDown += new System.Windows.Forms.KeyEventHandler(this.interestBox_KeyDown);
                    // 
                    // interestListView
                    // 
                    this.interestListView.BackColor = System.Drawing.SystemColors.Info;
                    this.interestListView.Columns.Add(this.InterestColumnHeader);
                    this.interestListView.Font = new System.Drawing.Font("Tahoma", 11F, System.Drawing.FontStyle.Regular);
                    this.interestListView.FullRowSelect = true;
                    this.interestListView.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
                    this.interestListView.Location = new System.Drawing.Point(15, 83);
                    this.interestListView.Name = "interestListView";
                    this.interestListView.Size = new System.Drawing.Size(206, 172);
                    this.interestListView.TabIndex = 2;
                    this.interestListView.View = System.Windows.Forms.View.List;
                    this.interestListView.SelectedIndexChanged += new System.EventHandler(this.interestListView_SelectedIndexChanged);
                    this.interestListView.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.interestListView_KeyPress);
                    this.interestListView.GotFocus += new System.EventHandler(this.interestListView_GotFocus);
                    // 
                    // InterestColumnHeader
                    // 
                    this.InterestColumnHeader.Text = "Interest";
                    this.InterestColumnHeader.Width = 150;
                    // 
                    // label1
                    // 
                    this.label1.Font = new System.Drawing.Font("Tahoma", 9F, System.Drawing.FontStyle.Bold);
                    this.label1.Location = new System.Drawing.Point(14, 2);
                    this.label1.Name = "label1";
                    this.label1.Size = new System.Drawing.Size(120, 21);
                    this.label1.Text = "New interest";
                    this.label1.ParentChanged += new System.EventHandler(this.label1_ParentChanged);
                    // 
                    // interestListLabel
                    // 
                    this.interestListLabel.Font = new System.Drawing.Font("Tahoma", 12F, System.Drawing.FontStyle.Bold);
                    this.interestListLabel.Location = new System.Drawing.Point(15, 56);
                    this.interestListLabel.Name = "interestListLabel";
                    this.interestListLabel.Size = new System.Drawing.Size(206, 24);
                    this.interestListLabel.Text = "My Interests";
                    this.interestListLabel.TextAlign = System.Drawing.ContentAlignment.TopCenter;
                    // 
                    // AddInterestWindow
                    // 
                    this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
                    this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
                    this.AutoScroll = true;
                    this.ClientSize = new System.Drawing.Size(240, 268);
                    this.Controls.Add(this.interestListLabel);
                    this.Controls.Add(this.label1);
                    this.Controls.Add(this.interestListView);
                    this.Controls.Add(this.interestTextBox);
                    this.KeyPreview = true;
                    this.Menu = this.mainMenu1;
                    this.Name = "AddInterestWindow";
                    this.Text = "PhotoShare: Add Interests";
                    this.Load += new System.EventHandler(this.AddInterestWindow_Load);
                    this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.AddInterestWindow_KeyDown);
                    this.ResumeLayout(false);

                }

                #endregion

                private System.Windows.Forms.MainMenu mainMenu1;
                private System.Windows.Forms.MenuItem menuItem1;
                private System.Windows.Forms.MenuItem menuItem2;
                private System.Windows.Forms.TextBox interestTextBox;
                public System.Windows.Forms.ListView interestListView;
                private System.Windows.Forms.Label label1;
                private System.Windows.Forms.Label interestListLabel;
                private System.Windows.Forms.ColumnHeader InterestColumnHeader;


        }
}