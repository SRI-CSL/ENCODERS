/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#ifndef _SCRATCHPAD_MANAGER_H
#define _SCRATCHPAD_MANAGER_H

#include "Scratchpad.h"

class ScratchpadManager;

class ScratchpadManager 
{
private:
    ScratchpadString *scratchpadString;
    ScratchpadDouble *scratchpadDouble;
    Mutex scratchpad_lock;
public:
    ScratchpadManager();
    virtual ~ScratchpadManager();
    
    bool hasScratchpadAttributeString(string dataobject_id, string name);
    bool hasScratchpadAttributeString(DataObjectRef &dObj, string name);

    void setScratchpadAttributeString(string dataobject_id, string name, string value);
    void setScratchpadAttributeString(DataObjectRef& dObj, string name, string value);

    string getScratchpadAttributeString(string dataobject_id, string name);
    string getScratchpadAttributeString(DataObjectRef &dObj, string name);

    void setScratchpadAttributeDouble(string dataobject_id, string name, double value);
    void setScratchpadAttributeDouble(DataObjectRef& dObj, string name, double value);

    bool hasScratchpadAttributeDouble(string dataobject_id, string name);
    bool hasScratchpadAttributeDouble(DataObjectRef &dObj, string name);

    double getScratchpadAttributeDouble(string dataobject_id, string name);
    double getScratchpadAttributeDouble(DataObjectRef &dObj, string name);

    void removeScratchpadAttribute(string dataobject_id, string name);
    void removeScratchpadAttribute(DataObjectRef &dObj, string name);

    void removeScratchpadDataObject(string dataobject_id);
    void removeScratchpadDataObject(DataObjectRef &dObj);
};

#endif /* _SCRATCHPAD_MANAGER_H */
