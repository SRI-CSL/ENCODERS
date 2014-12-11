/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 */

#include "CachePurger.h"

CachePurger::CachePurger(
    DataManager *m,
    const string name) :
        ManagerModule<DataManager>(m, name)
{
}

CachePurger::~CachePurger() 
{
}

void 
CachePurger::onConfig(
    const Metadata& m)
{
}

bool 
CachePurger::isResponsibleForDataObject(
    DataObjectRef &dObj) 
{
    return false; 
}

void
CachePurger::schedulePurge(
    DataObjectRef &dObj) 
{
    return; 
}

void
CachePurger::initializationPurge()
{
    return; 
}
