/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "ScratchpadManager.h"

ScratchpadManager::ScratchpadManager() 
    : scratchpadString(new ScratchpadString()),
      scratchpadDouble(new ScratchpadDouble())
{
}

ScratchpadManager::~ScratchpadManager()
{
    if (scratchpadString) {
        delete scratchpadString;
    }

    if (scratchpadDouble) {
        delete scratchpadDouble;
    }
}

void
ScratchpadManager::setScratchpadAttributeString(string dataobject_id, string name, string value)
{
    Mutex::AutoLocker l(scratchpad_lock);
    if (scratchpadDouble->hasScratchpadAttribute(dataobject_id, name)) {
        scratchpadDouble->removeScratchpadAttribute(dataobject_id, name);
    }
    scratchpadString->setScratchpadAttribute(dataobject_id, name, value);
}

void
ScratchpadManager::setScratchpadAttributeString(DataObjectRef &dObj, string name, string value)
{
    Mutex::AutoLocker l(scratchpad_lock);
    if (scratchpadDouble->hasScratchpadAttribute(dObj, name)) {
        scratchpadDouble->removeScratchpadAttribute(dObj, name);
    }
    scratchpadString->setScratchpadAttribute(dObj, name, value);
}

bool
ScratchpadManager::hasScratchpadAttributeString(string dataobject_id, string name)
{
    Mutex::AutoLocker l(scratchpad_lock);
    return scratchpadString->hasScratchpadAttribute(dataobject_id, name);
}

bool
ScratchpadManager::hasScratchpadAttributeString(DataObjectRef &dObj, string name)
{
    Mutex::AutoLocker l(scratchpad_lock);
    return scratchpadString->hasScratchpadAttribute(dObj, name);
}

string
ScratchpadManager::getScratchpadAttributeString(string dataobject_id, string name)
{
    Mutex::AutoLocker l(scratchpad_lock);
    return scratchpadString->getScratchpadAttribute(dataobject_id, name);
}

string
ScratchpadManager::getScratchpadAttributeString(DataObjectRef &dObj, string name)
{
    Mutex::AutoLocker l(scratchpad_lock);
    return scratchpadString->getScratchpadAttribute(dObj, name);
}

void 
ScratchpadManager::setScratchpadAttributeDouble(string dataobject_id, string name, double value)
{
    Mutex::AutoLocker l(scratchpad_lock);
    if (scratchpadString->hasScratchpadAttribute(dataobject_id, name)) {
        scratchpadString->removeScratchpadAttribute(dataobject_id, name);
    }
    scratchpadDouble->setScratchpadAttribute(dataobject_id, name, value);
}

void 
ScratchpadManager::setScratchpadAttributeDouble(DataObjectRef &dObj, string name, double value)
{
    Mutex::AutoLocker l(scratchpad_lock);
    if (scratchpadString->hasScratchpadAttribute(dObj, name)) {
        scratchpadString->removeScratchpadAttribute(dObj, name);
    }
    scratchpadDouble->setScratchpadAttribute(dObj, name, value);
}

bool
ScratchpadManager::hasScratchpadAttributeDouble(string dataobject_id, string name)
{
    Mutex::AutoLocker l(scratchpad_lock);
    return scratchpadDouble->hasScratchpadAttribute(dataobject_id, name);
}

bool
ScratchpadManager::hasScratchpadAttributeDouble(DataObjectRef &dObj, string name)
{
    Mutex::AutoLocker l(scratchpad_lock);
    return scratchpadDouble->hasScratchpadAttribute(dObj, name);
}

double
ScratchpadManager::getScratchpadAttributeDouble(string dataobject_id, string name)
{
    Mutex::AutoLocker l(scratchpad_lock);
    return scratchpadDouble->getScratchpadAttribute(dataobject_id, name);
}

double 
ScratchpadManager::getScratchpadAttributeDouble(DataObjectRef &dObj, string name)
{
    Mutex::AutoLocker l(scratchpad_lock);
    return scratchpadDouble->getScratchpadAttribute(dObj, name);
}

void
ScratchpadManager::removeScratchpadAttribute(string dataobject_id, string name)
{
    Mutex::AutoLocker l(scratchpad_lock);
    scratchpadDouble->removeScratchpadAttribute(dataobject_id, name);
    scratchpadString->removeScratchpadAttribute(dataobject_id, name);
}

void
ScratchpadManager::removeScratchpadAttribute(DataObjectRef &dObj, string name)
{
    Mutex::AutoLocker l(scratchpad_lock);
    scratchpadDouble->removeScratchpadAttribute(dObj, name);
    scratchpadString->removeScratchpadAttribute(dObj, name);
}

void 
ScratchpadManager::removeScratchpadDataObject(string dataobject_id)
{
    Mutex::AutoLocker l(scratchpad_lock);
    scratchpadDouble->removeScratchpadDataObject(dataobject_id);
    scratchpadString->removeScratchpadDataObject(dataobject_id);
}

void
ScratchpadManager::removeScratchpadDataObject(DataObjectRef &dObj)
{
    Mutex::AutoLocker l(scratchpad_lock);
    scratchpadDouble->removeScratchpadDataObject(dObj);
    scratchpadString->removeScratchpadDataObject(dObj);
}
