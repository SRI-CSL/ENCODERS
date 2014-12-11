/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */


#ifndef _FORWARDINGCLASSIFIER_SIZERANGE_H
#define _FORWARDINGCLASSIFIER_SIZERANGE_H

class ForwardingClassifierSizeRange;

#include "ForwardingClassifier.h"

class ForwardingClassifierSizeRange : public ForwardingClassifier {
#define FORWARDING_CLASSIFIER_SIZERANGE_NAME "ForwardingClassifierSizeRange"
#define FORWARDING_CLASSIFIER_SIZERANGE_OPTION_NAME "class_name"
#define FORWARDING_CLASSIFIER_SIZERANGE_MINB_NAME "min_bytes"
#define FORWARDING_CLASSIFIER_SIZERANGE_MAXB_NAME "max_bytes"
private:
   bool initialized;
   int minBytes;
   int maxBytes;
   string className;
   HaggleKernel *kernel;
protected:
   const string getClassNameForDataObject(const DataObjectRef &dObj);
   void onForwardingClassifierConfig(const Metadata &m);
public:
   ForwardingClassifierSizeRange(ForwardingManager *m = NULL);
   ~ForwardingClassifierSizeRange();
};

#endif /* FORWARDINGCLASSIFIER_SIZERANGE */
