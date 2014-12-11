/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */


#ifndef _PROTOCOLCLASSIFIER_ALLMATCH_H
#define _PROTOCOLCLASSIFIER_ALLMATCH_H

class ProtocolClassifierAllMatch;

#include "ProtocolClassifier.h"

class ProtocolClassifierAllMatch : public ProtocolClassifier {
#define PROTOCOL_CLASSIFIER_ALLMATCH_NAME "ProtocolClassifierAllMatch"
#define PROTOCOL_CLASSIFIER_ALLMATCH_OUT_TAG_NAME "outputTag"
private:
    bool initialized;
    string outputTag;
    HaggleKernel *kernel;
protected:
    string getClassNameForDataObject(const DataObjectRef &dObj);
    void onProtocolClassifierConfig(const Metadata &m);
public:
    ProtocolClassifierAllMatch(ProtocolManager *m = NULL);
    ~ProtocolClassifierAllMatch();
};

#endif /* PROTOCOLCLASSIFIER_ALLMATCH */
