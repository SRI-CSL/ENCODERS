/*
 * Copyright (c) 2012 GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (jjoy)
 */

#ifndef CSVUTILITY_H_
#define CSVUTILITY_H_

#include <vector>
#include <sstream>
#include <algorithm>

/*
#include <libcpphaggle/String.h>
#include <libcpphaggle/Map.h>

Map<std::string, bool> splitmap(const std::string &s, char delim, Map<std::string, bool> &elems);
Map<std::string, bool> splitmap(const std::string &s, char delim);
*/

bool itemInVector(std::string item,std::vector<std::string> elems);

std::vector<std::string> split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);

class CSVUtility {
public:
    CSVUtility();
    virtual ~CSVUtility();
};

#endif /* CSVUTILITY_H_ */
