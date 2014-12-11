/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Hasnain Lakhani (HL)
 */

#include "CacheStrategy.h"

CacheStrategy::CacheStrategy(
    DataManager *m,
    const string name) :
        ManagerModule<DataManager>(m, name)
{
}

void CacheStrategy::onConfig(const Metadata& m)
{
    return;
}

bool CacheStrategy::isResponsibleForDataObject(DataObjectRef &dObj)
{
    return false;
}

void CacheStrategy::handleNewDataObject(DataObjectRef &dObj)
{
    return;
}

void CacheStrategy::handleSendSuccess(DataObjectRef &dObj, NodeRef &node)
{
    return;
}

void CacheStrategy::handleDeletedDataObject(DataObjectRef &dObj)
{
    return;
}

void CacheStrategy::quit()
{
    return;
}

void CacheStrategy::printDebug()
{
    return;
}

// CBMEN, HL, Begin
void CacheStrategy::getCacheStrategyAsMetadata(Metadata *m)
{
    return;
}
// CBMEN, HL, End
