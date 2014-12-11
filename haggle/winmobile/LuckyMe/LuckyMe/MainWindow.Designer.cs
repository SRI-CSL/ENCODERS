namespace LuckyGUI
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
                        this.menuHide = new System.Windows.Forms.MenuItem();
                        this.luckyMeStatusLabel = new System.Windows.Forms.Label();
                        this.numNeighborsLabel = new System.Windows.Forms.Label();
                        this.haggleStatusLabel = new System.Windows.Forms.Label();
                        this.testStatusLabel = new System.Windows.Forms.Label();
                        this.neighborsButton = new System.Windows.Forms.Button();
                        this.lockButton = new System.Windows.Forms.Button();
                        this.lockStatusLabel = new System.Windows.Forms.Label();
                        this.numDataObjectsGenLabel = new System.Windows.Forms.Label();
                        this.numDataObjectsLabel = new System.Windows.Forms.Label();
                        this.SuspendLayout();
                        // 
                        // mainMenu1
                        // 
                        this.mainMenu1.MenuItems.Add(this.menuItem1);
                        this.mainMenu1.MenuItems.Add(this.menuHide);
                        // 
                        // menuItem1
                        // 
                        this.menuItem1.Text = "Controls";
                        this.menuItem1.Click += new System.EventHandler(this.controlsMenuItem_Click);
                        // 
                        // menuHide
                        // 
                        this.menuHide.Text = "Hide";
                        this.menuHide.Click += new System.EventHandler(this.menuHide_Click);
                        // 
                        // luckyMeStatusLabel
                        // 
                        this.luckyMeStatusLabel.Font = new System.Drawing.Font("Tahoma", 11F, System.Drawing.FontStyle.Regular);
                        this.luckyMeStatusLabel.Location = new System.Drawing.Point(4, 50);
                        this.luckyMeStatusLabel.Name = "luckyMeStatusLabel";
                        this.luckyMeStatusLabel.Size = new System.Drawing.Size(221, 22);
                        this.luckyMeStatusLabel.Text = "LuckyMe is not running";
                        // 
                        // numNeighborsLabel
                        // 
                        this.numNeighborsLabel.Font = new System.Drawing.Font("Tahoma", 11F, System.Drawing.FontStyle.Regular);
                        this.numNeighborsLabel.Location = new System.Drawing.Point(4, 129);
                        this.numNeighborsLabel.Name = "numNeighborsLabel";
                        this.numNeighborsLabel.Size = new System.Drawing.Size(221, 22);
                        this.numNeighborsLabel.Text = "0 neighbors";
                        // 
                        // haggleStatusLabel
                        // 
                        this.haggleStatusLabel.Font = new System.Drawing.Font("Tahoma", 11F, System.Drawing.FontStyle.Regular);
                        this.haggleStatusLabel.Location = new System.Drawing.Point(4, 28);
                        this.haggleStatusLabel.Name = "haggleStatusLabel";
                        this.haggleStatusLabel.Size = new System.Drawing.Size(221, 22);
                        this.haggleStatusLabel.Text = "Haggle is not running";
                        // 
                        // testStatusLabel
                        // 
                        this.testStatusLabel.Font = new System.Drawing.Font("Tahoma", 11F, System.Drawing.FontStyle.Regular);
                        this.testStatusLabel.Location = new System.Drawing.Point(4, 8);
                        this.testStatusLabel.Name = "testStatusLabel";
                        this.testStatusLabel.Size = new System.Drawing.Size(221, 20);
                        this.testStatusLabel.Text = "Test stage: NOT RUNNING";
                        // 
                        // neighborsButton
                        // 
                        this.neighborsButton.Font = new System.Drawing.Font("Tahoma", 11F, System.Drawing.FontStyle.Bold);
                        this.neighborsButton.Location = new System.Drawing.Point(129, 199);
                        this.neighborsButton.Name = "neighborsButton";
                        this.neighborsButton.Size = new System.Drawing.Size(104, 42);
                        this.neighborsButton.TabIndex = 5;
                        this.neighborsButton.Text = "Neighbors";
                        this.neighborsButton.Click += new System.EventHandler(this.neighborsButton_Click);
                        // 
                        // lockButton
                        // 
                        this.lockButton.Enabled = false;
                        this.lockButton.Font = new System.Drawing.Font("Tahoma", 11F, System.Drawing.FontStyle.Bold);
                        this.lockButton.Location = new System.Drawing.Point(8, 199);
                        this.lockButton.Name = "lockButton";
                        this.lockButton.Size = new System.Drawing.Size(108, 42);
                        this.lockButton.TabIndex = 6;
                        this.lockButton.Text = "Lock phone";
                        this.lockButton.Click += new System.EventHandler(this.lockButton_Click);
                        // 
                        // lockStatusLabel
                        // 
                        this.lockStatusLabel.Location = new System.Drawing.Point(8, 176);
                        this.lockStatusLabel.Name = "lockStatusLabel";
                        this.lockStatusLabel.Size = new System.Drawing.Size(108, 20);
                        this.lockStatusLabel.Text = "Phone is unlocked";
                        this.lockStatusLabel.ParentChanged += new System.EventHandler(this.label6_ParentChanged);
                        // 
                        // numDataObjectsGenLabel
                        // 
                        this.numDataObjectsGenLabel.Font = new System.Drawing.Font("Tahoma", 11F, System.Drawing.FontStyle.Regular);
                        this.numDataObjectsGenLabel.Location = new System.Drawing.Point(4, 106);
                        this.numDataObjectsGenLabel.Name = "numDataObjectsGenLabel";
                        this.numDataObjectsGenLabel.Size = new System.Drawing.Size(221, 20);
                        this.numDataObjectsGenLabel.Text = "0 data objects generated";
                        this.numDataObjectsGenLabel.ParentChanged += new System.EventHandler(this.numDataObjectsGenLabel_ParentChanged);
                        // 
                        // numDataObjectsLabel
                        // 
                        this.numDataObjectsLabel.Font = new System.Drawing.Font("Tahoma", 11F, System.Drawing.FontStyle.Regular);
                        this.numDataObjectsLabel.Location = new System.Drawing.Point(4, 84);
                        this.numDataObjectsLabel.Name = "numDataObjectsLabel";
                        this.numDataObjectsLabel.Size = new System.Drawing.Size(221, 22);
                        this.numDataObjectsLabel.Text = "0 data objects received";
                        this.numDataObjectsLabel.ParentChanged += new System.EventHandler(this.numDataObjectsLabel_ParentChanged);
                        // 
                        // MainWindow
                        // 
                        this.AutoScaleDimensions = new System.Drawing.SizeF(96F, 96F);
                        this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Dpi;
                        this.AutoScroll = true;
                        this.ClientSize = new System.Drawing.Size(240, 268);
                        this.Controls.Add(this.numDataObjectsGenLabel);
                        this.Controls.Add(this.lockStatusLabel);
                        this.Controls.Add(this.lockButton);
                        this.Controls.Add(this.neighborsButton);
                        this.Controls.Add(this.testStatusLabel);
                        this.Controls.Add(this.haggleStatusLabel);
                        this.Controls.Add(this.numNeighborsLabel);
                        this.Controls.Add(this.numDataObjectsLabel);
                        this.Controls.Add(this.luckyMeStatusLabel);
                        this.KeyPreview = true;
                        this.Menu = this.mainMenu1;
                        this.Name = "MainWindow";
                        this.Text = "LuckyMe";
                        this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.MainWindow_KeyDown);
                        this.ResumeLayout(false);

                }

                #endregion

                private System.Windows.Forms.Label luckyMeStatusLabel;
                private System.Windows.Forms.Label numNeighborsLabel;
                private System.Windows.Forms.MenuItem menuHide;
                private System.Windows.Forms.Label haggleStatusLabel;
                private System.Windows.Forms.Label testStatusLabel;
                private System.Windows.Forms.Button neighborsButton;
                private System.Windows.Forms.MenuItem menuItem1;
                private System.Windows.Forms.Button lockButton;
                private System.Windows.Forms.Label lockStatusLabel;
                private System.Windows.Forms.Label numDataObjectsGenLabel;
                private System.Windows.Forms.Label numDataObjectsLabel;
        }
}

