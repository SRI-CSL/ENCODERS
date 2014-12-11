namespace PhotoShare
{
        partial class NeighborListWindow
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
                    this.menuItemBack = new System.Windows.Forms.MenuItem();
                    this.neighborListView = new System.Windows.Forms.ListView();
                    this.columnHeaderName = new System.Windows.Forms.ColumnHeader();
                    this.columnHeaderInterfaces = new System.Windows.Forms.ColumnHeader();
                    this.SuspendLayout();
                    // 
                    // mainMenu1
                    // 
                    this.mainMenu1.MenuItems.Add(this.menuItemBack);
                    // 
                    // menuItemBack
                    // 
                    this.menuItemBack.Text = "Back";
                    this.menuItemBack.Click += new System.EventHandler(this.menuItemBack_Click);
                    // 
                    // neighborListView
                    // 
                    this.neighborListView.Columns.Add(this.columnHeaderName);
                    this.neighborListView.Columns.Add(this.columnHeaderInterfaces);
                    this.neighborListView.Dock = System.Windows.Forms.DockStyle.Fill;
                    this.neighborListView.Location = new System.Drawing.Point(0, 0);
                    this.neighborListView.Name = "neighborListView";
                    this.neighborListView.Size = new System.Drawing.Size(240, 268);
                    this.neighborListView.TabIndex = 0;
                    this.neighborListView.View = System.Windows.Forms.View.Details;
                    this.neighborListView.SelectedIndexChanged += new System.EventHandler(this.neighborListView_SelectedIndexChanged);
                    // 
                    // columnHeaderName
                    // 
                    this.columnHeaderName.Text = "Name";
                    this.columnHeaderName.Width = 126;
                    // 
                    // columnHeaderInterfaces
                    // 
                    this.columnHeaderInterfaces.Text = "Interfaces";
                    this.columnHeaderInterfaces.Width = 117;
                    // 
                    // NeighborListWindow
                    // 
                    this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
                    this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
                    this.AutoScroll = true;
                    this.ClientSize = new System.Drawing.Size(240, 268);
                    this.Controls.Add(this.neighborListView);
                    this.KeyPreview = true;
                    this.Menu = this.mainMenu1;
                    this.Name = "NeighborListWindow";
                    this.Text = "PhotoShare: Neighbors";
                    this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.NeighborListWindow_KeyDown);
                    this.ResumeLayout(false);

                }

                #endregion

                private System.Windows.Forms.MenuItem menuItemBack;
                private System.Windows.Forms.ColumnHeader columnHeaderName;
                private System.Windows.Forms.ColumnHeader columnHeaderInterfaces;
                public System.Windows.Forms.ListView neighborListView;
        }
}