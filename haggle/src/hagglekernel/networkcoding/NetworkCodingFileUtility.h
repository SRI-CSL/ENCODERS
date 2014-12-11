/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef NETWORKCODINGFILEUTILITY_H_
#define NETWORKCODINGFILEUTILITY_H_

#include <libcpphaggle/String.h>

class NetworkCodingFileUtility {
public:
    NetworkCodingFileUtility();
    virtual ~NetworkCodingFileUtility();

    string createNetworkCodedBlockFileName(string originalFileName);
    string createDecodedBlockFileName(const char*networkCodedFileName);
    string createDecodedBlockFilePath(const char* decodedFileName,const char* storagePath);
};

#endif /* NETWORKCODINGFILEUTILITY_H_ */
