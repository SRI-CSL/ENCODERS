/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */


#ifndef _PROTOCOLCLASSIFIERBASIC_H
#define _PROTOCOLCLASSIFIERBASIC_H

class ProtocolClassifierBasic;

#include "ProtocolClassifier.h"

class ProtocolClassifierBasic : public ProtocolClassifier {
#define PROTOCOL_CLASSIFIER_BASIC_NAME "ProtocolClassifierBasic"
#define PROTOCOL_CLASSIFIER_BASIC_LIGHTWEIGHT "lightWeightClassName"
#define PROTOCOL_CLASSIFIER_BASIC_HEAVYWEIGHT "heavyWeightClassName"
private:
    bool initialized;
    string lightWeightClassName;
    string heavyWeightClassName;
    HaggleKernel *kernel;
protected:
    string getClassNameForDataObject(const DataObjectRef &dObj);
    void onProtocolClassifierConfig(const Metadata &m);
public:
    ProtocolClassifierBasic(ProtocolManager *m = NULL);
    ~ProtocolClassifierBasic();
};

#endif /* PROTOCOLCLASSIFIERBASIC */
