/* Copyright (c) 2014 SRI International and Suns-tech Incorporated
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   James Mathewson (JM, JLM)
 *   Jihwa Lee (JL)
 */


#include "CacheReplacementPriority.h"

#include "CacheReplacementFactory.h"

CacheReplacementPriority::CacheReplacementPriority(DataManager *m)
   : CacheReplacement(m, CACHE_REPLACEMENT_PRIORITY_NAME) 
{
}

CacheReplacementPriority::~CacheReplacementPriority() 
{
    List<Pair<CacheReplacement *, int> >::iterator it = sorted_replacement_list.begin();

    for (; it != sorted_replacement_list.end(); it++) {
        delete (*it).first;
    }
}

static void
sortedReplacementListInsert(
    List<Pair<CacheReplacement *, int> >& list, 
    CacheReplacement *replacement, 
    int priority)
{
    List<Pair<CacheReplacement *, int> >::iterator it = list.begin();

    for (; it != list.end(); it++) {
        if (priority > (*it).second) {
            break;
        }
    }
    list.insert(it, make_pair(replacement, priority));
}

void 
CacheReplacementPriority::onConfig(
    const Metadata& m)
{
    if (m.getName() != getName()) {
        HAGGLE_ERR("Wrong config.\n");
        return;
    }

    int i = 0;
    const char *param = NULL;
    const Metadata *replacementConfig;
    CacheReplacement *replacement;
    while (NULL != (replacementConfig = m.getMetadata("CacheReplacement", i++))) {
        param = replacementConfig->getParameter(CACHE_REPLACEMENT_PRIORITY_NAME_FIELD_NAME);
        if (!param) {
            HAGGLE_ERR("No replacement class specified\n");
            return;
        }

        replacement = CacheReplacementFactory::getNewCacheReplacement(getManager(), string(param));

        if (!replacement) {
            HAGGLE_ERR("Could not instantiate replacement\n");
            return;
        }

        const Metadata *options = replacementConfig->getMetadata(param);
        if (!options) {
            delete replacement;
            HAGGLE_ERR("Could not get configuration options for replacement\n");
            return;
        }
        
        replacement->onConfig(*options);

        param = replacementConfig->getParameter(CACHE_REPLACEMENT_PRIORITY_FIELD_NAME);
        if (!param) {
            HAGGLE_ERR("No replacement class specified\n");
            return;
        }

        int priority = 0;
        char *endptr = NULL;
        int paramInt = (int) strtol (param, &endptr, 10);
        if (endptr && endptr != param && paramInt > 0) {
            priority = paramInt;
        }

        if (0 >= priority) { 
            delete replacement;
            HAGGLE_ERR("Could not assign priority to replacement\n");
            return;
        }

        sortedReplacementListInsert(sorted_replacement_list, replacement, priority);
    }
}

bool 
CacheReplacementPriority::isResponsibleForDataObject(
    DataObjectRef &dObj) 
{
    List<Pair<CacheReplacement *, int> >::iterator it = sorted_replacement_list.begin();

    for (; it != sorted_replacement_list.end(); it++) {
        CacheReplacement *replacement = (*it).first;
        if (replacement->isResponsibleForDataObject(dObj)) {
            return true;
        }
    }

    return false; 
}

static DataObjectRefList 
intersectDataObjectList(DataObjectRefList a, DataObjectRefList b)
{
    // naive O(n^2) intersection, not relying on dObj order
    DataObjectRefList intersection = DataObjectRefList();
    for (DataObjectRefList::iterator itA = a.begin(); itA != a.end(); itA++) {
        for (DataObjectRefList::iterator itB = b.begin(); itB != b.end(); itB++) {
            if (*itB == *itA) {
                intersection.add(*itA);
            }
        }
    }
    return intersection;
}

static DataObjectRefList 
unionDataObjectList(DataObjectRefList a, DataObjectRefList b)
{
    // naive O(n^2) union, not relying on dObj order
    DataObjectRefList setUnion = DataObjectRefList();
    for (DataObjectRefList::iterator itA = a.begin(); itA != a.end(); itA++) {
        setUnion.add(*itA);
    }

    for (DataObjectRefList::iterator itB = b.begin(); itB != b.end(); itB++) {
        bool inA = false;
        for (DataObjectRefList::iterator itA = a.begin(); itA != a.end(); itA++) {
            if (*itB == *itA) {
                inA = true;
            }
        }

        if (!inA) {
            setUnion.add(*itB);
        }
    }

    return setUnion;
}

static bool
anyRelevant(
    DataObjectRef dObj, 
    List<CacheReplacement *> *relevantReplacements) 
{
    List<CacheReplacement* >::iterator it = 
            relevantReplacements->begin();

    for (; it != relevantReplacements->end(); it++) {
        CacheReplacement *replacement = *it;
        if (replacement->isResponsibleForDataObject(dObj)) {
            return true;
        }
    }

    return false;
}

static bool
allRelevant(
    DataObjectRef dObj, 
    List<CacheReplacement *> *relevantReplacements) 
{
    List<CacheReplacement* >::iterator it = 
            relevantReplacements->begin();

    bool hasItem = false;
    for (; it != relevantReplacements->end(); it++) {
        hasItem = true;
        CacheReplacement *replacement = *it;
        if (!(replacement->isResponsibleForDataObject(dObj))) {
            return false;
        }
    }

    return hasItem && true;
}

static DataObjectRefList 
filterDataObjectListByReplacementList(
    DataObjectRefList a, 
    List<CacheReplacement *> *relevantReplacements,
    List<CacheReplacement *> *irrelevantReplacements)
{
    DataObjectRefList filterdObjs = DataObjectRefList();
    for (DataObjectRefList::iterator itA = a.begin(); itA != a.end(); itA++) {
        DataObjectRef dObj = *itA; 

        if (allRelevant(dObj, relevantReplacements) 
            && !anyRelevant(dObj, irrelevantReplacements)) {
            filterdObjs.add(dObj);
        }
    }

    return filterdObjs;
}

static List<CacheReplacement *> *
filterRelevantReplacementListByDataObject(
    DataObjectRef dObj,  
    List<Pair<CacheReplacement *, int> > *replacements) 
{
    List<CacheReplacement* > *filteredReplacements = new List<CacheReplacement* >();

    List<Pair<CacheReplacement*, int> >::iterator itA = replacements->begin();
    for (; itA != replacements->end(); itA++) {
        CacheReplacement *replacement = (*itA).first;
        if (replacement->isResponsibleForDataObject(dObj)) {
            filteredReplacements->push_back(replacement);
        }
    }

    return filteredReplacements;
}

static List<CacheReplacement *> *
filterIrrelevantReplacementListByDataObject(
    DataObjectRef dObj,  
    List<Pair<CacheReplacement *, int> > *replacements) 
{
    List<CacheReplacement* > *filteredReplacements = new List<CacheReplacement* >();

    List<Pair<CacheReplacement*, int> >::iterator itA = replacements->begin();
    for (; itA != replacements->end(); itA++) {
        CacheReplacement *replacement = (*itA).first;
        if (!(replacement->isResponsibleForDataObject(dObj))) {
            filteredReplacements->push_back(replacement);
        }
    }

    return filteredReplacements;
}

void
CacheReplacementPriority::getOrganizedDataObjectsByNewDataObject(
    DataObjectRef &dObj,
    DataObjectRefList *o_subsumed,
    DataObjectRefList *o_equiv,
    DataObjectRefList *o_nonsubsumed,
    bool &isDatabaseTimeout)    
{
    if (NULL == o_subsumed || NULL == o_equiv || NULL == o_nonsubsumed) {
        HAGGLE_ERR("Null args\n");
        return;
    }

    bool firstIteration = false;
    DataObjectRefList equivdObjs = DataObjectRefList();
    DataObjectRefList subsumeddObjs = DataObjectRefList();
    DataObjectRefList nonSubsumeddObjs = DataObjectRefList();

    List<CacheReplacement *> *relevantReplacements = 
        filterRelevantReplacementListByDataObject(
            dObj, 
            &sorted_replacement_list);

    List<CacheReplacement *> *irrelevantReplacements = 
        filterIrrelevantReplacementListByDataObject(
            dObj, 
            &sorted_replacement_list);

    List<CacheReplacement *>::iterator it = relevantReplacements->begin();

    for (; it != relevantReplacements->end(); it++) {
        CacheReplacement *replacement = *it;

        DataObjectRefList currentSubsumeddObjs;
        DataObjectRefList currentEquivdObjs;
        DataObjectRefList currentNonSubsumeddObjs;

        replacement->getOrganizedDataObjectsByNewDataObject(
            dObj, 
            &currentSubsumeddObjs,
            &currentEquivdObjs,
            &currentNonSubsumeddObjs,
            isDatabaseTimeout);            

        currentSubsumeddObjs = filterDataObjectListByReplacementList(
            currentSubsumeddObjs,
            relevantReplacements,
            irrelevantReplacements);

        currentEquivdObjs = filterDataObjectListByReplacementList(
            currentEquivdObjs,
            relevantReplacements,
            irrelevantReplacements);

        currentNonSubsumeddObjs = filterDataObjectListByReplacementList(
            currentNonSubsumeddObjs,
            relevantReplacements,
            irrelevantReplacements);

        if (!firstIteration) {
            firstIteration = true;
            equivdObjs = currentEquivdObjs;
            subsumeddObjs = currentSubsumeddObjs;
            nonSubsumeddObjs = currentNonSubsumeddObjs;
            continue;
        }

        subsumeddObjs = unionDataObjectList(
            subsumeddObjs, 
            intersectDataObjectList(equivdObjs, currentSubsumeddObjs));

        nonSubsumeddObjs = unionDataObjectList(
            nonSubsumeddObjs, 
            intersectDataObjectList(equivdObjs, currentNonSubsumeddObjs));

        equivdObjs = intersectDataObjectList(
            equivdObjs, 
            currentEquivdObjs);
    }

    delete relevantReplacements;
    delete irrelevantReplacements;

    (*o_subsumed) = subsumeddObjs;
    (*o_equiv) = equivdObjs;
    (*o_nonsubsumed) = nonSubsumeddObjs;
}
