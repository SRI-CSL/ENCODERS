/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */


#ifndef _PROTOCOLCLASSIFIERND_H
#define _PROTOCOLCLASSIFIERND_H

class ProtocolClassifierNodeDescription;

#include "ProtocolClassifier.h"

class ProtocolClassifierNodeDescription : public ProtocolClassifier {
#define PROTOCOL_CLASSIFIER_ND_NAME "ProtocolClassifierNodeDescription"
#define PROTOCOL_CLASSIFIER_ND_OUT_TAG_NAME "outputTag"
private:
    bool initialized;
    string outputTag;
    HaggleKernel *kernel;
protected:
    string getClassNameForDataObject(const DataObjectRef &dObj);
    void onProtocolClassifierConfig(const Metadata &m);
public:
    ProtocolClassifierNodeDescription(ProtocolManager *m = NULL);
    ~ProtocolClassifierNodeDescription();
};

#endif /* PROTOCOLCLASSIFIERND */
