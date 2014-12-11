/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include <libcpphaggle/String.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "NetworkCodingFileUtility.h"
#include <Trace.h>

NetworkCodingFileUtility::NetworkCodingFileUtility() {

}

NetworkCodingFileUtility::~NetworkCodingFileUtility() {

}

string time_stamp() {

    char *timestamp = (char *) malloc(sizeof(char) * 80);
    memset(timestamp,0,sizeof(timestamp));
    time_t ltime;
    ltime = time(NULL);
    struct tm *tm;
    tm = localtime(&ltime);
    struct timeval tv;
    gettimeofday(&tv,NULL);


    sprintf(timestamp, "%04d%02d%02d%02d%02d%02d%d", tm->tm_year + 1900, tm->tm_mon, tm->tm_mday, tm->tm_hour, tm->tm_min,
            tm->tm_sec,tv.tv_usec);
    string stringtimestamp = timestamp;
    free(timestamp);
    return stringtimestamp;
}

string NetworkCodingFileUtility::createNetworkCodedBlockFileName(string originalFileName) {
    string storage = HAGGLE_DEFAULT_STORAGE_PATH;
    string extension = ".nc";
    string timestamp = time_stamp();

    HAGGLE_DBG2("originalFileName=%s timestamp=%s \n", originalFileName.c_str(),timestamp.c_str());

    string networkCodedBlockFileName = storage + "/"+ originalFileName + "."+timestamp+extension;

    HAGGLE_DBG2("networkCodedBlockFileName=%s\n",networkCodedBlockFileName.c_str());

    return networkCodedBlockFileName;
}

string NetworkCodingFileUtility::createDecodedBlockFileName(const char*originalFileName) {
    int fname_len = strlen(originalFileName);

    string timestamp = time_stamp();
    string ext = "."+timestamp+".dc";
    size_t ext_len = ext.length();

    HAGGLE_DBG2("Creating block file name from networkCodedFileName=%s fnamelen=%d extlen=%d\n",originalFileName,fname_len,ext_len);

    char* decodedBlockFilePtr = (char*) malloc(sizeof(char *) * 256);
    char* const decodedBlockFile = decodedBlockFilePtr;
    memset(decodedBlockFilePtr, 0, sizeof(decodedBlockFilePtr));

    strcpy(decodedBlockFilePtr, originalFileName);
    decodedBlockFilePtr = decodedBlockFilePtr + fname_len;
    strcpy(decodedBlockFilePtr, ext.c_str());
    decodedBlockFilePtr = decodedBlockFilePtr + ext_len;
    //strcpy(decodedBlockFilePtr, 0);
    HAGGLE_DBG2("Result: decodedFile=%s\n", decodedBlockFile);
    string decodedBlockFileName = decodedBlockFile;
    free(decodedBlockFile);
    return decodedBlockFileName;
}

string NetworkCodingFileUtility::createDecodedBlockFilePath(const char* decodedFileName,const char* storagePath) {
    int decodedFileNameLen = strlen(decodedFileName);
    int storagePathLen = strlen(storagePath);

    HAGGLE_DBG2("Creating block file path from decodedFileName=%s storagePath=%s decodedFileNameLen=%d storagePathLen=%d\n",decodedFileName,storagePath,decodedFileNameLen,storagePathLen);

    char* decodedBlockFilePathPtr = (char*) malloc(sizeof(char *) * 256);
    char* const decodedBlockFilePath = decodedBlockFilePathPtr;
    memset(decodedBlockFilePathPtr, 0, sizeof(decodedBlockFilePathPtr));

    strcpy(decodedBlockFilePathPtr,storagePath);
    decodedBlockFilePathPtr = decodedBlockFilePathPtr + storagePathLen;
    strcpy(decodedBlockFilePathPtr,"/");
    decodedBlockFilePathPtr = decodedBlockFilePathPtr+1;
    strcpy(decodedBlockFilePathPtr,decodedFileName);
    HAGGLE_DBG2("Result: decodedFilePath=%s\n",decodedBlockFilePath);
    string decodedBlockFilePathName = decodedBlockFilePath;
    free(decodedBlockFilePath);
    return decodedBlockFilePathName;
}
