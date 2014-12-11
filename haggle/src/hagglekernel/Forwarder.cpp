/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Mark-Oliver Stehr (MOS)
 *   Sam Wood (SW)
 */

/* Copyright 2009 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Forwarder.h"

DataObjectRef Forwarder::createRoutingInformationDataObject()
{
	// No need to have a reference in this function because it won't be 
	// visible outside until this function is done with it.
	DataObjectRef dObj = DataObject::create();
	
	if (!dObj)
		return NULL;

	HAGGLE_DBG2("Creating routing info data object\n");

	dObj->setPersistent(false);
	dObj->addAttribute("Forwarding", getName());
	
	// Add the metric data to the forwarding section:
	Metadata *md = dObj->getMetadata()->addMetadata(getManager()->getName());
	
	md = md->addMetadata(getName());
	md->setParameter("node_id", getKernel()->getThisNode()->getIdStr());
	
	if (!addRoutingInformation(dObj, md)) {
        // SW: ERR->DBG, this is not fatal, we return false when we don't want
        // to add routing data.
	        // HAGGLE_DBG("Could not add routing information\n");
		return NULL;
	}

	return dObj;
}

// SW: check if forwarder is interested in this repository data
bool Forwarder::isReleventRepositoryData(RepositoryEntryRef &re)
{
    return 0 == strcmp(re->getAuthority(), getName());
}

// SW: get the repository data for the forwarder
void Forwarder::getRepositoryData(EventCallback<EventHandler> *repositoryCallback)
{
    if (NULL == repositoryCallback) {
        return; 
    }

    HAGGLE_DBG("Searching for repository data for forwarder:%s\n", getName());

    getManager()->getKernel()->getDataStore()->readRepository(new RepositoryEntry(getName()), repositoryCallback);
}

bool Forwarder::hasRoutingInformation(const DataObjectRef& dObj)
{
	if (!dObj)
		return false;
	
	const Metadata *m = dObj->getMetadata()->getMetadata(getManager()->getName());
	
	if (m == NULL)
		return false;
		
	if (!m->getMetadata(getName()))
		return false;
	    
	return true;
}

const string Forwarder::getNodeIdFromRoutingInformation(const DataObjectRef& dObj) const
{
	if (!dObj)
		return (char *)NULL;
	
	const Metadata *m = dObj->getMetadata()->getMetadata(getManager()->getName());
	
	if (!m)
		return (char *)NULL;
	
	m = m->getMetadata(getName());
	
	if (!m)
		return (char *)NULL;
	
	return m->getParameter("node_id");
}

const Metadata *Forwarder::getRoutingInformation(const DataObjectRef& dObj) const
{
	if (!dObj)
		return NULL;
	
	const Metadata *md = dObj->getMetadata();
	
	if (md == NULL)
		return NULL;
	
	md = md->getMetadata(getManager()->getName());
	
	if (md == NULL)
		return NULL;
	
	md = md->getMetadata(getName());
	
	if (md == NULL)
		return NULL;
	
	return md;
}

bool Forwarder::isTarget(const NodeRef &delegate, const NodeRefList *targets) const
{
	if (!targets)
		return false;

	for (NodeRefList::const_iterator it = targets->begin(); it != targets->end(); it++) {
		if (*it == delegate)
			return true;
	}
	return false;
}

void Forwarder::onConfig(const Metadata& m) 
{
	if (m.getName().compare("Forwarder") != 0)
		return;
	
	const char *param = m.getParameter("max_generated_delegates");
	
	if (param) {
		char *ptr = NULL;
		unsigned long d = strtoul(param, &ptr, 10);
		
		if (ptr && ptr != param && *ptr == '\0') {
			HAGGLE_DBG("%s Setting max_generated_delegates to %lu\n", getName(), d);
			max_generated_delegates = d;
		}
	}
	
	param = m.getParameter("max_generated_targets");
	
	if (param) {
		char *ptr = NULL;
		unsigned long d = strtoul(param, &ptr, 10);
		
		if (ptr && ptr != param && *ptr == '\0') {
			HAGGLE_DBG("%s Setting max_generated_targets to %lu\n", getName(), d);
			max_generated_targets = d;
		}
	}
	
	const Metadata *md = m.getMetadata(getName());
	
	if (md) {
		onForwarderConfig(*md);
	}
}
