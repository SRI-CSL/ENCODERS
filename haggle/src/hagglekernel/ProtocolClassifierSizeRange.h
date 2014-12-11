/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */


#ifndef _PROTOCOLCLASSIFIER_SIZERANGE_H
#define _PROTOCOLCLASSIFIER_SIZERANGE_H

class ProtocolClassifierSizeRange;

#include "ProtocolClassifier.h"

class ProtocolClassifierSizeRange : public ProtocolClassifier {
#define PROTOCOL_CLASSIFIER_SIZERANGE_NAME "ProtocolClassifierSizeRange"
#define PROTOCOL_CLASSIFIER_SIZERANGE_OUT_TAG_NAME "outputTag"
#define PROTOCOL_CLASSIFIER_SIZERANGE_MINB_NAME "minBytes"
#define PROTOCOL_CLASSIFIER_SIZERANGE_MAXB_NAME "maxBytes"
private:
    bool initialized;
    int minBytes;
    int maxBytes;
    string outputTag;
    HaggleKernel *kernel;
protected:
    string getClassNameForDataObject(const DataObjectRef &dObj);
    void onProtocolClassifierConfig(const Metadata &m);
public:
    ProtocolClassifierSizeRange(ProtocolManager *m = NULL);
    ~ProtocolClassifierSizeRange();
};

#endif /* PROTOCOLCLASSIFIER_SIZERANGE */
