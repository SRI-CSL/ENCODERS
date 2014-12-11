/*
 * Copyright (c) 2012 GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (jjoy)
 */

#include "CSVUtility.h"

#include <string>

CSVUtility::CSVUtility() {


}

CSVUtility::~CSVUtility() {

}

/*
Map<std::string, bool> splitmap(const std::string &s, char delim, Map<std::string, bool> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        //elems.push_back(item);
        std::string key = "";
        elems[key]=true;
        //elems[item]=true;
    }
    return elems;
}


Map<std::string, bool> splitmap(const std::string &s, char delim) {
    Map<std::string, bool> elems;
    return splitmap(s, delim, elems);
}
*/

bool itemInVector(std::string item,std::vector<std::string> elems) {
    if( std::find(elems.begin(), elems.end(), item)!=elems.end() ) {
        return true;
    }
    return false;
}



std::vector<std::string> split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    return split(s, delim, elems);
}
