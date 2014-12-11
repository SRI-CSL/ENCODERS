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
using System.Threading;
using System.IO;
using System.Diagnostics;
using System.Reflection;
using Microsoft.WindowsCE.Forms;
using Microsoft.WindowsMobile.Forms;
using Microsoft.WindowsMobile.Status;
using Haggle;

namespace PhotoShare
{
        public partial class MainWindow : Form
        {
                public object photoListLock = new object();

                PhotoShare ps;

                string defaultPictureFileName = "haggle-img";

                public MainWindow(PhotoShare _ps)
                {
                        ps = _ps;
                       
                        InitializeComponent();

                        /*
                         * The following code is for loading the color of the banner so you can
                         * run the same version of photoshare on all test cell phones and still 
                         * have different colors.
                        */
                        bool hasLoadedColor = false;

                        FileStream fs = null;
                        try
                        {
                            fs = System.IO.File.OpenRead(Assembly.GetExecutingAssembly().GetName().CodeBase.
                                Replace("PhotoShare.exe", "") + "pssettings.txt");
                        }
                        catch 
                        {
                                Debug.WriteLine("pssettings.txt not found");
                        }

                        if (fs != null)
                        {
                            byte[] data = new byte[fs.Length];
                            fs.Read(data, 0, (int)fs.Length);
                            fs.Close();

                            String str = Utility.ByteArrayToString(data);
                            char[] lineendings = { '\n' };
                            String[] line = str.Split(lineendings);
                            int i;
                            for (i = 0; i < line.Length; i++)
                                if (line[i].Length > 12 && !hasLoadedColor)
                                    if (String.Compare(line[i], 0, "BannerColor=", 0, 12) == 0)
                                    {
                                        char[] commas = { ',' };
                                        string[] value = line[i].Substring(12).Split(commas);
                                        if (value.Length == 3)
                                        {
                                            myPicturesLabel.BackColor =
                                                Color.FromArgb(
                                                    int.Parse(value[0]),
                                                    int.Parse(value[1]),
                                                    int.Parse(value[2]));
                                            hasLoadedColor = true;
                                        }
                                    }
                        }

                        // Calculate size of thumbnails of pictures on main screen
                        // assuming we should layout two thumbnails per row

                        // Account for some space at edges and inbetween thumbnails (15% of width each)
                        
                        double width = (this.Width - (this.Width * 0.15) * 3) / 2;

                        Debug.WriteLine("thumbnail width is " + (int)width);
                        photoImageList.ImageSize = new System.Drawing.Size((int)width, (int)width);
                }
        
                private static Image resizeImage(Image imgToResize, Size size)
                {

                        int sourceWidth = imgToResize.Width;
                        int sourceHeight = imgToResize.Height;

                        float nPercent = 0;
                        float nPercentW = 0;
                        float nPercentH = 0;

                        nPercentW = ((float)size.Width / (float)sourceWidth);
                        nPercentH = ((float)size.Height / (float)sourceHeight);

                        if (nPercentH < nPercentW)
                                nPercent = nPercentH;
                        else
                                nPercent = nPercentW;

                        int destWidth = (int)(sourceWidth * nPercent);
                        int destHeight = (int)(sourceHeight * nPercent);
                        Bitmap b = new Bitmap(destWidth, destHeight);
                        Graphics g = Graphics.FromImage((Image)b);
                        g.DrawImage(imgToResize, 0, 0, new Rectangle(0, 0, destWidth, destHeight), GraphicsUnit.Pixel);
                        g.Dispose();
                        return (Image)b;
                }
                // This will run on a thread associated with the list handle
                public void doPhotoListUpdate(Haggle.DataObject dObj)
                {
                        Debug.WriteLine("doPhotoListUpdate");

                        string filepath = dObj.GetFilePath();
                        string filename = dObj.GetFileName();
                        string attribute = dObj.GetAttribute("Picture").GetValue();

                        Debug.WriteLine("Received photo " + filepath);

                        lock (photoListLock)
                        {
                                // Add photo to image list

                            ListViewItem lvi = new ListViewItem(attribute);

                                // If the image is too big we will get an out of memory
                                // exception. We need to find a good way to scale images.
                                try
                                {
                                        photoImageList.Images.Add(new Bitmap(filepath));
                                        lvi.ImageIndex = photoImageList.Images.Count - 1;

                                        photoListView.BeginUpdate();

                                        photoListView.Items.Add(lvi);

                                        photoListView.EndUpdate();
                                }
                                catch (Exception)
                                {
                                        Debug.WriteLine("Could not create bitmap from image " + filepath);
                                }

                        }
                }
                private void menuQuit_Click(object sender, EventArgs e)
                {
                        ps.quit();
                }

                private void menuTakePicture_Click(object sender, EventArgs e)
                {
                        CameraCaptureDialog cameraCapture = new CameraCaptureDialog();
                        cameraCapture.Owner = this;

                        object cameraEnabled = Microsoft.WindowsMobile.Status.SystemState.GetValue(Microsoft.WindowsMobile.Status.SystemProperty.CameraEnabled);
                        
                        cameraCapture.DefaultFileName = "haggle-temp.jpg";
                        cameraCapture.Title = "Take PhotoShare Picture";
                        cameraCapture.Mode = CameraCaptureMode.Still;
                        cameraCapture.Resolution = new Size(1024, 768);
                       
                        // The filename of the picure taken
                        string fileName;

                        if (null != cameraEnabled && 0 == (int)cameraEnabled)
                        {
                                MessageBox.Show("The camera is disabled", this.Text,
                                        MessageBoxButtons.OK, MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1);
                                return;
                        }

                        try
                        {
                                if (cameraCapture.ShowDialog() == DialogResult.OK)
                                {
                                        fileName = cameraCapture.FileName;

                                        Debug.WriteLine("file name of photo is " + fileName);

                                        if (cameraCapture.Mode == CameraCaptureMode.Still)
                                        {
                                                string extension = fileName.Substring(fileName.LastIndexOf("."));
                                                string directory = "";
                                                long extra_digit;

                                                if (fileName.LastIndexOf("\\") != -1)
                                                {
                                                        directory = fileName.Substring(0, fileName.LastIndexOf("\\") + 1);
                                                }

                                                fileName = directory + this.defaultPictureFileName + extension;
                                                extra_digit = 0;
                                                while (System.IO.File.Exists(fileName))
                                                {
                                                    fileName = directory + this.defaultPictureFileName + "-" + extra_digit + extension;
                                                    extra_digit++;
                                                }

                                                System.IO.File.Move(cameraCapture.FileName, fileName);
                                        }
                                        
                                        FileAttributeWindow fileAttrWin = new FileAttributeWindow(fileName);
                                        DialogResult ret = DialogResult.Cancel;

                                        try {
                                                ret = fileAttrWin.ShowDialog();
                                        } catch (Exception) {
                                                Debug.WriteLine("Could not show fileAttrWind dialog");
                                        }
                                        //fileAttrWin.BringToFront();
                                        char[] separators = { ';', ' ' };

                                        string[] keywords = fileAttrWin.getResultValueString().Split(separators);

                                        Debug.WriteLine("Attributes string is: " + fileAttrWin.getResultValueString());

                                        //DialogResult ret = MessageBox.Show("The picture was saved to:\n" + fileName + 
                                        //        "\nDo you want to publish the picture with Haggle now?",
                                        //    this.Text, MessageBoxButtons.YesNo, MessageBoxIcon.Asterisk, MessageBoxDefaultButton.Button1);
                                        
                                        if (ret == DialogResult.OK)
                                        {
                                                try
                                                {
                                                        Haggle.Attribute.AttributeList al = new Haggle.Attribute.AttributeList();

                                                        Haggle.DataObject dObj = new Haggle.DataObject(fileName);
                                                        dObj.AddHash();
                                                        // Add thumbnail:
                                                        dObj.SetThumbnail(GetThumbnail(fileName, 32, 0));
                                                        
                                                        foreach (string kw in keywords)
                                                        {
                                                                Haggle.Attribute a = new Haggle.Attribute("Picture", kw);

                                                                if (!dObj.AddAttribute(a))
                                                                {
                                                                        MessageBox.Show("Could not add attribute");
                                                                        return;
                                                                }

                                                                if (fileAttrWin.getAddAsInterest())
                                                                {

                                                                        if (ps.addInterestWindow.interestListUpdate(new Haggle.Attribute.AttributeList(a)) > 0)
                                                                        {
                                                                                Debug.WriteLine("Adding interest " + a.ToString());
                                                                                al.Add(a);
                                                                        }
                                                                }

                                                        }
                                                        // We need to add the interest first if we want the filters to match the
                                                        // data object we add
                                                        if (fileAttrWin.getAddAsInterest())
                                                        {
                                                                ps.hh.AddInterests(al);
                                                        }
                                                        //Haggle.Attribute attr = dObj.GetAttribute("DeviceName");

                                                        //MessageBox.Show("Data object file name is: " + dObj.GetFileName()
                                                        //               + " Attribute is: " + attr.GetValue());
                                                        int sent = ps.hh.PublishDataObject(dObj);

                                                        if (sent < 0)
                                                        {
                                                                MessageBox.Show("Could not publish data object on handle=" +
                                                                        ps.hh.handle + " Error=" + sent);
                                                        }
                                                }
                                                catch (Haggle.DataObject.NoSuchAttributeException)
                                                {
                                                        MessageBox.Show("No such attribute in data object");
                                                }
                                                catch (Haggle.DataObject.DataObjectException ex)
                                                {
                                                        MessageBox.Show(ex.ToString());
                                                }
                                                catch (Haggle.Attribute.AttributeNullPtrException)
                                                {
                                                        MessageBox.Show("Attribute null pointer exception");
                                                }
                                                catch (Exception)
                                                {
                                                        MessageBox.Show("Unknown error");
                                                }
                                        }
                                        else if (ret == DialogResult.No)
                                        {

                                                Debug.WriteLine("camera dialog result \'NO\'");
                                        }
                                        else
                                        {
                                                Debug.WriteLine("Unknown camera dialog result");
                                                MessageBox.Show("Unknown selection.");
                                        }
                                        Debug.WriteLine("camera exit");
                                }
                        }
                        catch (ArgumentException ex)
                        {
                                MessageBox.Show(ex.Message, this.Text, MessageBoxButtons.OK,
                                    MessageBoxIcon.Exclamation, MessageBoxDefaultButton.Button1);
                        }
                        catch (OutOfMemoryException ex)
                        {
                                MessageBox.Show(ex.Message, this.Text, MessageBoxButtons.OK,
                                    MessageBoxIcon.Hand, MessageBoxDefaultButton.Button1);
                        }
                        catch (InvalidOperationException ex)
                        {
                                MessageBox.Show(ex.Message, this.Text, MessageBoxButtons.OK,
                                    MessageBoxIcon.Hand, MessageBoxDefaultButton.Button1);
                        }

                }
                
                /*
                 * GetThumbnail
                 * 
                 * Similar to CreateThumbnail, except that it returns a byte array with the 
                 * thumbnail encoded as a jpeg.
                 */
                private static byte[] GetThumbnail(
                        string originalFilename,
                        long width,
                        long height)
                {
                    string directory = "";
                    if (originalFilename.LastIndexOf("\\") != -1)
                    {
                        directory = originalFilename.Substring(0, originalFilename.LastIndexOf("\\") + 1);
                    }
                    Bitmap thumbnail = CreateThumbnail(originalFilename, 64, 0);
                    string thumbnailFile = directory + "thumb.jpg";
                    thumbnail.Save(thumbnailFile, System.Drawing.Imaging.ImageFormat.Jpeg);
                    FileStream fs = System.IO.File.OpenRead(thumbnailFile);
                    byte[] data = new byte[fs.Length];
                    fs.Read(data, 0, data.Length);
                    fs.Close();
                    try
                    {
                        System.IO.File.Delete(thumbnailFile);
                    }
                    catch { }
                    return data;
                }
                /*
                 * CreateThumbnail.
                 * 
                 * Creates a thumbnail image based on a file on disk.
                 * 
                 * originalFilename: the name of a file on disk to create a thumbnail of.
                 * width: the width of the thumbnail.
                 * height: the height of the thumbnail or 0, if the function should make sure to keep
                 *         the aspect ratio of the original image.
                 * 
                */
                private static Bitmap CreateThumbnail(
                        string originalFilename,
                        long width,
                        long height)
                {
                    Bitmap retval = null;

                    try
                    {
                        Bitmap original = new Bitmap(originalFilename);

                        if (width < 0 || height < 0)
                            return null;

                        if (height == 0)
                        {
                            double ratio;
                            ratio = ((double)original.Height) / ((double)original.Width);
                            height = ((long)(ratio * width));
                        }

                        retval = new Bitmap((int)width, (int)height);
                        Graphics g = Graphics.FromImage(retval);
                        g.DrawImage(
                                original,
                                new Rectangle(0, 0, (int)width, (int)height),
                                new Rectangle(0, 0, original.Width, original.Height),
                                System.Drawing.GraphicsUnit.Pixel);
                        original.Dispose();
                    }
                    catch
                    {
                        retval = null;
                    }
                    return retval;
                }

                private void statusBar1_ParentChanged_1(object sender, EventArgs e)
                {

                }

                private void listView1_SelectedIndexChanged(object sender, EventArgs e)
                {

                }

                private void shutdownHaggle_Click(object sender, EventArgs e)
                {
                        ps.shutdown();
                }

                private void label1_ParentChanged_1(object sender, EventArgs e)
                {

                }

                private void menuAddInterest_Click(object sender, EventArgs e)
                {
                        DialogResult ret = DialogResult.None;

                        try
                        {
                                ret = ps.addInterestWindow.ShowDialog();
                        }
                        catch (Exception)
                        {
                                Debug.WriteLine("Could not show addInterest dialog");
                        }

                        if (ret == DialogResult.OK)
                        {
                                int retval = 0;

                                Haggle.Attribute.AttributeList al = ps.addInterestWindow.getAddInterests();

                                foreach (Haggle.Attribute a in al.AsArray())
                                {
                                        Debug.WriteLine("Add interest: " + a.ToString());
                                }

                                if (al.Size() > 0)
                                {
                                        retval = ps.hh.AddInterests(al);

                                        Debug.WriteLine("Add interests returned: " + retval);
                                }
                                al = ps.addInterestWindow.getDelInterests();

                                foreach (Haggle.Attribute a in al.AsArray())
                                {
                                        Debug.WriteLine("Delete interest: " + a.ToString());
                                }

                                if (al.Size() > 0)
                                {
                                        retval = ps.hh.DeleteInterests(al);

                                        Debug.WriteLine("Delete interests returned: " + retval);

                                        photoListView.Clear();
                                        ps.dataObjects.Clear();
                                        ps.addInterestWindow.interestListView.Clear();
                                        ps.hh.RequestInterests();
                                        ps.hh.RequestDataObjects();
                                }
                        }
                }

                private void PhotoShareForm_Load(object sender, EventArgs e)
                {

                }

                private void menuItemViewNeighbors_Click(object sender, EventArgs e)
                {
                        DialogResult ret = DialogResult.None;

                        try
                        {
                                ret = ps.neighborListWindow.ShowDialog();
                        }
                        catch (Exception)
                        {
                                Debug.WriteLine("Could not show neighbor list dialog");
                        }

                        if (ret == DialogResult.OK)
                        {

                        }
                }
                private void photoListView_SelectedIndexChanged(object sender, EventArgs e)
                {
                        DialogResult ret = DialogResult.OK;
                        Haggle.DataObject dObj;
                        Int32 selectedIndex = -1;

                        if (photoListView.SelectedIndices.Count != 1)
                        {
                                return;
                        }

                        selectedIndex = photoListView.SelectedIndices[0];

                        Debug.WriteLine("Clicked image " + selectedIndex);
                        Debug.WriteLine("Selected count is " + photoListView.SelectedIndices.Count);

                        dObj = ps.dataObjects[selectedIndex];

                        Debug.WriteLine("Picture filename is " + dObj.GetFileName());
                        Debug.WriteLine("Picture filepath is " + dObj.GetFilePath());
                        try
                        {
                                ShowImageWindow imgWin = new ShowImageWindow(new Bitmap(dObj.GetFilePath()), dObj.GetFileName());
                                ret = imgWin.ShowDialog();
                        }
                        catch (Exception)
                        {
                                Debug.WriteLine("Could not show image window dialog");
                                return;
                        }

                        if (ret == DialogResult.Yes)
                        {
                                // Yes means we should delete the image.
                                Debug.WriteLine("Deleting picture with index " + selectedIndex + " and filename " + dObj.GetFileName());
                                ps.hh.DeleteDataObject(dObj);
                                ps.dataObjects.RemoveAt(selectedIndex);
                                photoListView.Items.RemoveAt(selectedIndex);
                        }
                }

                private void menuItem1_Click(object sender, EventArgs e)
                {

                }

                private void label1_ParentChanged(object sender, EventArgs e)
                {

                }
        }
}
