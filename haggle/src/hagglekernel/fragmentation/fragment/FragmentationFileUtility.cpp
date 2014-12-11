/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */


#include "FragmentationFileUtility.h"
#include "Trace.h"

string gentime_stamp() {

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

string FragmentationFileUtility::createFragmentFileName(string originalFileName) {
	string storage = HAGGLE_DEFAULT_STORAGE_PATH;
    string extension = ".frag";
    string timestamp = gentime_stamp();

    HAGGLE_DBG2("Creating fragment file name from originalFileName=%s timestamp=%s \n", originalFileName.c_str(),timestamp.c_str());

    string fragementationFileName = storage + "/" + originalFileName + "." + timestamp + extension;

    HAGGLE_DBG2("Result: fragmentationFileName=%s\n",fragementationFileName.c_str());

    return fragementationFileName;
}
