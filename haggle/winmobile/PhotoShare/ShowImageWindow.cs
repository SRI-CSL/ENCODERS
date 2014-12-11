using System;
using System.Linq;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Text;
using System.Windows.Forms;

namespace PhotoShare
{
        public partial class ShowImageWindow : Form
        {
                public ShowImageWindow(Image img, string filename)
                {
                        InitializeComponent();
                        fileNameLabel.Text = filename;
                        pictureBox1.Image = img;
                        pictureBox1.Size = img.Size;
                        pictureBox1.SizeMode = PictureBoxSizeMode.StretchImage;
                }

                private void menuItem1_Click(object sender, EventArgs e)
                {

                        this.DialogResult = DialogResult.OK;
                        this.Close();
                }

                private void menuItem2_Click(object sender, EventArgs e)
                { 
                        DialogResult res = MessageBox.Show("Are you sure you want to delete the photo?",
                                                "Delete photo", MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button1);

                        if (res == DialogResult.Yes)
                        {
                                // Delete image.
                                this.DialogResult = DialogResult.Yes;
                        }
                        else
                        {
                                this.DialogResult = DialogResult.No;
                        }
                        this.Close();
                }
        }
}