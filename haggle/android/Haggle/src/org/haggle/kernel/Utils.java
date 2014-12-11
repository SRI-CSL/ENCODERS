/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Hasnain Lakhani (HL)
 */

/*
 * Copyright (C) 2010 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */

/*
 * Copyright (C) 2012, Anthony Prieur & Daniel Oppenheim. All rights reserved.
 *
 * Original from SL4A modified to allow to embed Interpreter and scripts into an APK
 */

/*
* Copyright (C) 2013, Muhammad Hasnain Lakhani. All rights reserved.
*
* Original from android-python27 modified to log to Haggle and include chmod from SL4A FileUtils
*/

package org.haggle.kernel;

import android.os.Environment;
import android.util.Log;
import android.content.ContextWrapper;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.zip.ZipInputStream;
import java.lang.reflect.Method;

public class Utils {
    public static String getFileExtension(String sFileName) {
      int dotIndex = sFileName.lastIndexOf('.');
      if (dotIndex == -1) {
        return null;
      }
      return sFileName.substring(dotIndex);
    }

    // Taken from SL4A FileUtils
    public static int chmod(File path, int mode) throws Exception {
      Class<?> fileUtils = Class.forName("android.os.FileUtils");
      Method setPermissions =
          fileUtils.getMethod("setPermissions", String.class, int.class, int.class, int.class);
      return (Integer) setPermissions.invoke(null, path.getAbsolutePath(), mode, -1, -1);
    }
 
    //-------------------------------------------------------------------------------------------------

      public static boolean unzip(InputStream inputStream, String dest, boolean replaceIfExists) {
          
          final int BUFFER_SIZE = 4096;
          
          BufferedOutputStream bufferedOutputStream = null;
          
          boolean succeed = true;
          
          try {
              ZipInputStream zipInputStream = new ZipInputStream(new BufferedInputStream(inputStream));
              ZipEntry zipEntry;
              
              while ((zipEntry = zipInputStream.getNextEntry()) != null){
               
               String zipEntryName = zipEntry.getName();
               
               
//             if(!zipEntry.isDirectory()) {
//                 File fil = new File(dest + zipEntryName);
//                 fil.getParent()
//             }
               
               // file exists ? delete ?
               File file2 = new File(dest + zipEntryName);

               if(file2.exists()) {
                    if (replaceIfExists) {
                        
                       try {
                          boolean b = deleteDir(file2);
                              if(!b) {
                                    Log.e("Haggle", "Unzip failed to delete " + dest + zipEntryName);
                              }
                              else {
                                    Log.d("Haggle", "Unzip deleted " + dest + zipEntryName);
                              }
                        } catch (Exception e) {
                            Log.e("Haggle", "Unzip failed to delete " + dest + zipEntryName, e);
                        }
                    }              
               }

               // extract
               File file = new File(dest + zipEntryName);
               
               if (file.exists()){

               } else {
                if(zipEntry.isDirectory()){
                 file.mkdirs(); 
                 chmod(file, 0755);

                }else{
                    
                 // create parent file folder if not exists yet
                 if(!file.getParentFile().exists()) {
                      file.getParentFile().mkdirs(); 
                      chmod(file.getParentFile(), 0755);
                 }
                       
                 byte buffer[] = new byte[BUFFER_SIZE];
                 bufferedOutputStream = new BufferedOutputStream(new FileOutputStream(file), BUFFER_SIZE);
                 int count;

                 while ((count = zipInputStream.read(buffer, 0, BUFFER_SIZE)) != -1) {
                  bufferedOutputStream.write(buffer, 0, count);
                 }

                 bufferedOutputStream.flush();
                 bufferedOutputStream.close(); 
                }
               }
               
               // enable standalone python
               if(file.getName().endsWith(".so") || file.getName().endsWith(".xml") || file.getName().endsWith(".py") || file.getName().endsWith(".pyc") || file.getName().endsWith(".pyo")) {
                   chmod(file, 0755);
               }

               Log.d("Haggle","Unzip extracted " + dest + zipEntryName);
              }
              
              zipInputStream.close();

             } catch (FileNotFoundException e) {
                 Log.e("Haggle","Unzip error, file not found", e);
                 succeed = false;
             }catch (Exception e) {
                 Log.e("Haggle","Unzip error: ", e);
                 succeed = false;
             }
            
             return succeed;             
      }
      
      //-------------------------------------------------------------------------------------------------

      public static boolean deleteDir(File dir) {
          try {
              if (dir.isDirectory()) {
                  String[] children = dir.list();
                  for (int i=0; i<children.length; i++) {
                      boolean success = deleteDir(new File(dir, children[i]));
                      if (!success) {
                          return false;
                      }
                  }
              }
          
              // The directory is now empty so delete it
              return dir.delete();
              
        } catch (Exception e) {
            Log.e("Haggle","Failed to delete " + dir + " : " + e);
            return false;
        }
      }
}
