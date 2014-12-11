namespace PhotoShare
{
        partial class ShowImageWindow
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
                    this.pictureBox1 = new System.Windows.Forms.PictureBox();
                    this.fileNameLabel = new System.Windows.Forms.Label();
                    this.SuspendLayout();
                    // 
                    // mainMenu1
                    // 
                    this.mainMenu1.MenuItems.Add(this.menuItem1);
                    this.mainMenu1.MenuItems.Add(this.menuItem2);
                    // 
                    // menuItem1
                    // 
                    this.menuItem1.Text = "Done";
                    this.menuItem1.Click += new System.EventHandler(this.menuItem1_Click);
                    // 
                    // menuItem2
                    // 
                    this.menuItem2.Text = "Delete";
                    this.menuItem2.Click += new System.EventHandler(this.menuItem2_Click);
                    // 
                    // pictureBox1
                    // 
                    this.pictureBox1.Dock = System.Windows.Forms.DockStyle.Fill;
                    this.pictureBox1.Location = new System.Drawing.Point(0, 0);
                    this.pictureBox1.Name = "pictureBox1";
                    this.pictureBox1.Size = new System.Drawing.Size(240, 268);
                    // 
                    // fileNameLabel
                    // 
                    this.fileNameLabel.Location = new System.Drawing.Point(0, 449);
                    this.fileNameLabel.Name = "fileNameLabel";
                    this.fileNameLabel.Size = new System.Drawing.Size(445, 48);
                    this.fileNameLabel.Text = "fileName";
                    // 
                    // ShowImageWindow
                    // 
                    this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
                    this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
                    this.ClientSize = new System.Drawing.Size(240, 268);
                    this.Controls.Add(this.fileNameLabel);
                    this.Controls.Add(this.pictureBox1);
                    this.Menu = this.mainMenu1;
                    this.Name = "ShowImageWindow";
                    this.Text = "PhotoShare";
                    this.ResumeLayout(false);

                }

                #endregion

                private System.Windows.Forms.MenuItem menuItem1;
                private System.Windows.Forms.MenuItem menuItem2;
                private System.Windows.Forms.PictureBox pictureBox1;
                private System.Windows.Forms.Label fileNameLabel;
        }
}