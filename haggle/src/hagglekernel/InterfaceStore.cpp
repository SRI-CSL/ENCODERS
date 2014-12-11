/* Copyright 2008-2009 Uppsala University
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
#include "InterfaceStore.h"
#include "Trace.h"
#include "Metadata.h"

InterfaceStore::InterfaceStore()
{
}

InterfaceStore::~InterfaceStore()
{
	int n = 0;

	while (!empty()) {
		InterfaceRecord *ir = front();
		ir->iface->resetFlag(IFFLAG_STORED);
		pop_front();
		delete ir;
		n++;
	}
	HAGGLE_DBG("Deleted %d interface records in interface store\n", n);
}

InterfaceStore::size_type InterfaceStore::remove_children(const InterfaceRef &parent, InterfaceRefList *ifl)
{
	InterfaceStore::iterator it = begin();
	InterfaceRefList children;
        int removed = 0;

	while (it != end()) {
		InterfaceRecord *ir = *it;

		if (ir->parent && ir->parent == parent) {
                        if (ifl)
                                ifl->add(ir->iface);

			ir->iface->resetFlag(IFFLAG_STORED);
			children.push_front(ir->iface);
			it = erase(it);
			delete ir;
                        removed++;
			continue;
		}
		it++;
	}

	while (!children.empty()) {
		removed += remove_children(children.pop(), ifl);
	}

	return removed;
}

bool InterfaceStore::stored(const Interface &iface)
{
        Mutex::AutoLocker l(mutex);

	for (InterfaceStore::iterator it = begin(); it != end(); it++) {
		InterfaceRecord *ir = *it;

		if (ir->iface == iface)
			return true;
	}

	return false;
}

bool InterfaceStore::stored(const InterfaceRef &iface)
{
        Mutex::AutoLocker l(mutex);

	if (!iface)
		return false;

	for (InterfaceStore::iterator it = begin(); it != end(); it++) {
		InterfaceRecord *ir = *it;

		if (ir->iface == iface)
			return true;
	}

	return false;
}

bool InterfaceStore::stored(Interface::Type_t type, const unsigned char *identifier)
{
        Mutex::AutoLocker l(mutex);

	if (!identifier)
		return false;

	for (InterfaceStore::iterator it = begin(); it != end(); it++) {
		InterfaceRecord *ir = *it;

		if (ir->iface->equal(type, identifier))
			return true;
	}

	return false;
}

InterfaceRef InterfaceStore::addupdate(InterfaceRef &iface, const InterfaceRef& parent, ConnectivityInterfacePolicy *policy, bool *was_added)
{
	InterfaceRef parentStore;

	if (!iface)
		return NULL;

	if (was_added)
		*was_added = false;

	// Always use the parent reference which is already in the store.
	// The exception is if parent == NULL, which means that we are
	// adding a parent interface (local connectivity).
	if (parent) {
		parentStore = retrieve(parent);

		if (!parentStore)
			return NULL;
	}

        Mutex::AutoLocker l(mutex);

	for (InterfaceStore::iterator it = begin(); it != end(); it++) {
		InterfaceRecord *ir = *it;
		InterfaceRef& ifaceStore = ir->iface;

		if (ir->iface == iface) {
			if (ir->cip)
				delete ir->cip;

			ir->cip = policy;

			if (ir->iface->isSnooped()) {
				ir->iface->resetFlag(IFFLAG_SNOOPED);
				ir->parent = parentStore;
			}

			if (ir->cip)
				ir->cip->update();

			return ifaceStore;
		}
	}

	InterfaceRecord *ir = new InterfaceRecord(iface, parentStore, policy);
	push_back(ir);

	if (was_added) {
		iface->setFlag(IFFLAG_STORED);
		*was_added = true;
	}

	return iface;
}

InterfaceRef InterfaceStore::addupdate(Interface *iface, const InterfaceRef &parent, ConnectivityInterfacePolicy *policy, bool *was_added)
{
	InterfaceRef parentRef;

	if (!iface)
		return NULL;

	if (was_added)
		*was_added = false;

	// Always use the parent reference which is already in the store.
	if (parent)
		parentRef = retrieve(parent);

        Mutex::AutoLocker l(mutex);

	for (InterfaceStore::iterator it = begin(); it != end(); it++) {
		InterfaceRecord *ir = *it;
		InterfaceRef& ifaceInStore = ir->iface;

		if (ir->iface == *iface) {
			if (ir->cip)
				delete ir->cip;

			ir->cip = policy;

			if (ir->iface->isSnooped()) {
				ir->iface->resetFlag(IFFLAG_SNOOPED);
				ir->parent = parentRef;
			}

			if (ir->cip)
				ir->cip->update();

			return ifaceInStore;
		}
	}

	InterfaceRef ifaceRef = iface->copy();
	ifaceRef->setFlag(IFFLAG_STORED);
	InterfaceRecord *ir = new InterfaceRecord(ifaceRef, parentRef, policy);
	push_back(ir);

	if (was_added)
		*was_added = true;

	return ifaceRef;
}

InterfaceRef InterfaceStore::addupdate(Interface *iface, const Interface *parent, ConnectivityInterfacePolicy *policy, bool *was_added)
{
	InterfaceRef parentStore;

	if (!iface)
		return NULL;

	if (was_added)
		*was_added = false;

	// Always use the parent reference which is already in the store.
	// The exception is if parent == NULL, which means that we are
	// adding a parent interface (local connectivity).
	if (parent) {
		parentStore = retrieve(*parent);

		if (!parentStore)
			return NULL;
	}

        Mutex::AutoLocker l(mutex);

	for (InterfaceStore::iterator it = begin(); it != end(); it++) {
		InterfaceRecord *ir = *it;
		InterfaceRef& ifaceStore = ir->iface;

		if (ir->iface == *iface) {
			if (ir->cip)
				delete ir->cip;

			ir->cip = policy;

			if (ir->iface->isSnooped()) {
				ir->iface->resetFlag(IFFLAG_SNOOPED);
				ir->parent = parentStore;
			}

			if (ir->cip)
				ir->cip->update();

			return ifaceStore;
		}
	}

	InterfaceRef ifaceRef = iface->copy();
	InterfaceRecord *ir = new InterfaceRecord(ifaceRef, parentStore, policy);
	push_back(ir);

	if (was_added) {
		ifaceRef->setFlag(IFFLAG_STORED);
		*was_added = true;
	}

	return ifaceRef;
}

InterfaceRef InterfaceStore::retrieve(const InterfaceRef &iface)
{
        Mutex::AutoLocker l(mutex);

	if (!iface)
		return NULL;

	for (InterfaceStore::iterator it = begin(); it != end(); it++) {
		InterfaceRecord *ir = *it;
		InterfaceRef &ifaceStore = ir->iface;

		if (ifaceStore == iface)
			return ifaceStore;
	}

	return NULL;
}

InterfaceRef InterfaceStore::retrieve(const Interface &iface)
{
        Mutex::AutoLocker l(mutex);

	for (InterfaceStore::iterator it = begin(); it != end(); it++) {
		InterfaceRecord *ir = *it;
		InterfaceRef &ifaceRef = ir->iface;

		if (ifaceRef == iface) {
			return ifaceRef;
		}
	}

	return NULL;
}

InterfaceRef InterfaceStore::retrieve(const Address &add)
{
        Mutex::AutoLocker l(mutex);

	for (InterfaceStore::iterator it = begin(); it != end(); it++) {
		InterfaceRecord *ir = *it;
		InterfaceRef &ifaceRef = ir->iface;

		if (ifaceRef->hasAddress(add))
			return ifaceRef;
	}

	return NULL;
}


InterfaceRef InterfaceStore::retrieve(Interface::Type_t type, const unsigned char *identifier)
{
        Mutex::AutoLocker l(mutex);

	if (!identifier)
		return NULL;

	for (InterfaceStore::iterator it = begin(); it != end(); it++) {
		InterfaceRecord *ir = *it;
		InterfaceRef &iface = ir->iface;

		if (iface->equal(type, identifier))
			return iface;
	}

	return NULL;
}

InterfaceStore::size_type InterfaceStore::retrieve(const Criteria& crit, InterfaceRefList& ifl)
{
        Mutex::AutoLocker l(mutex);
        size_type n = 0;

	for (InterfaceStore::iterator it = begin(); it != end(); it++) {
		const InterfaceRecord *ir = *it;

		if (crit(*ir)) {
			ifl.add(ir->iface);
                        n++;
		}
	}

	return n;
}

InterfaceRef InterfaceStore::retrieveParent(const InterfaceRef& iface)
{
	Mutex::AutoLocker l(mutex);

	for (InterfaceStore::iterator it = begin(); it != end(); it++) {
		const InterfaceRecord *ir = *it;

		if (ir->iface == iface) {
			return ir->parent;
		}
	}

	return NULL;
}

InterfaceStore::size_type InterfaceStore::remove(const string name, InterfaceRefList *ifl)
{
        Mutex::AutoLocker l(mutex);
        size_type removed = 0;

	InterfaceStore::iterator it = begin();

	while (it != end()) {
		InterfaceRecord *ir = *it;

		if (ir->iface->getName() == name) {
			it = erase(it);

                        if (ifl)
                                ifl->add(ir->iface);

			removed += remove_children(ir->iface, ifl) + 1;

			ir->iface->resetFlag(IFFLAG_STORED);
			delete ir;
			return removed;
		}
		it++;
	}

	return removed;
}

InterfaceStore::size_type InterfaceStore::remove(const Interface *iface, InterfaceRefList *ifl)
{
        Mutex::AutoLocker l(mutex);
        size_type removed = 0;

	if (!iface) {
		// This must be a request to remove children discovered by
		// a local connectivity, i.e., local interfaces.
		return remove_children(NULL, ifl);
	}

	InterfaceStore::iterator it = begin();

	while (it != end()) {
		InterfaceRecord *ir = *it;

		if (ir->iface == *iface) {
			it = erase(it);

			if (ifl)
                                ifl->add(ir->iface);

			removed += remove_children(ir->iface, ifl) + 1;

			ir->iface->resetFlag(IFFLAG_STORED);
			delete ir;
			return removed;
		}
		it++;
	}

	return removed;
}

InterfaceStore::size_type InterfaceStore::remove(const InterfaceRef &iface, InterfaceRefList *ifl)
{
        Mutex::AutoLocker l(mutex);
        size_type removed = 0;

	if (!iface) {
		// This must be a request to remove children discovered by
		// a local connectivity, i.e., local interfaces.
		return remove_children(iface, ifl);
	}

	InterfaceStore::iterator it = begin();

	while (it != end()) {
		InterfaceRecord *ir = *it;

		if (ir->iface == iface) {
			it = erase(it);

                        if (ifl)
                                ifl->add(ir->iface);

			removed += remove_children(ir->iface, ifl) + 1;

			ir->iface->resetFlag(IFFLAG_STORED);
			delete ir;
			return removed;
		}
		it++;
	}

	return removed;
}

InterfaceStore::size_type InterfaceStore::remove(Interface::Type_t type, const unsigned char *identifier, InterfaceRefList *ifl)
{
        Mutex::AutoLocker l(mutex);
        size_type removed = 0;

	if (!identifier) {
		// This must be a request to remove children discovered by
		// a local connectivity, i.e., local interfaces.
		return remove_children(NULL, ifl);
	}
	InterfaceStore::iterator it = begin();

	while (it != end()) {
		InterfaceRecord *ir = *it;

		if (ir->iface->equal(type, identifier)) {
			it = erase(it);

			removed += remove_children(ir->iface, ifl) + 1;

                        if (ifl)
                                ifl->add(ir->iface);

			ir->iface->resetFlag(IFFLAG_STORED);
			delete ir;
			return removed;
		}
		it++;
	}

	return removed;
}

InterfaceStore::size_type InterfaceStore::age(const Interface *parent, InterfaceRefList *ifl, Timeval *lifetime)
{
        Mutex::AutoLocker l(mutex);
	InterfaceRefList children;
        size_type removed = 0;

	// Initialize the lifetime to an "invalid" number
	if (lifetime)
		*lifetime = Timeval(-1);

	InterfaceStore::iterator it = begin();

	while (it != end()) {
		InterfaceRecord *ir = *it;

		if ((ir->iface->isLocal() && !parent) ||
		    // (parent && ir->parent && ir->parent == parent && !ir->iface->isSnooped())) {
		    // MOS - ageing should also be applied to snooped interfaces
		    (parent && ir->parent && ir->parent == parent)) {

			if (ir->cip->isDead()) {
				if (ifl)
                                        ifl->add(ir->iface);
                                removed++;

				ir->iface->resetFlag(IFFLAG_STORED);
				it = erase(it);
				children.push_front(ir->iface);

				delete ir;
				continue;
			}
			ir->cip->age();

			if (lifetime && ir->cip->lifetime().isValid()) {
				if (!lifetime->isValid() || ir->cip->lifetime() < *lifetime) {
					*lifetime = ir->cip->lifetime();
				}
			}
		}
		it++;
	}

	while (!children.empty()) {
		removed += remove_children(children.pop(), ifl);
	}

	return removed;
}

InterfaceStore::size_type InterfaceStore::age(Interface::Type_t type, const unsigned char *identifier, InterfaceRefList *ifl, Timeval *lifetime)
{
        Mutex::AutoLocker l(mutex);
	InterfaceRefList children;
        size_type removed = 0;

	// Initialize the lifetime to an "invalid" number
	if (lifetime)
		*lifetime = Timeval(-1);

	InterfaceStore::iterator it = begin();

	while (it != end()) {
		InterfaceRecord *ir = *it;

		if ((ir->iface->isLocal() && !identifier) ||
		    // ((identifier && ir->parent) && ir->parent->equal(type, identifier) && !ir->iface->isSnooped())) {
		    // MOS - ageing should also be applied to snooped interfaces
		    ((identifier && ir->parent) && ir->parent->equal(type, identifier))) {

			if (ir->cip->isDead()) {
				if (ifl)
                                        ifl->add(ir->iface);

                                removed++;
				ir->iface->resetFlag(IFFLAG_STORED);
				it = erase(it);
				children.push_front(ir->iface);

				delete ir;
				continue;
			}
			ir->cip->age();

			if (lifetime && ir->cip->lifetime().isValid()) {
				if (!lifetime->isValid() || ir->cip->lifetime() < *lifetime) {
					*lifetime = ir->cip->lifetime();
				}
			}
		}
		it++;
	}

	while (!children.empty()) {
		removed += remove_children(children.pop(), ifl);
	}

	return removed;
}


InterfaceStore::size_type InterfaceStore::age(const InterfaceRef &parent, InterfaceRefList *ifl, Timeval *lifetime)
{
        Mutex::AutoLocker l(mutex);
	InterfaceRefList children;
        size_type removed = 0;

	// Initialize the lifetime to an "invalid" number
	if (lifetime)
		*lifetime = Timeval(-1);

	InterfaceStore::iterator it = begin();

	while (it != end()) {
		InterfaceRecord *ir = *it;

		if ((ir->iface->isLocal() && !parent) ||
		    (parent && ir->parent && ir->parent == parent && !ir->iface->isSnooped())) {

			if (ir->cip->isDead()) {
				if (ifl)
                                        ifl->add(ir->iface);

                                removed++;
				ir->iface->resetFlag(IFFLAG_STORED);
				it = erase(it);
				children.push_front(ir->iface);

				delete ir;
				continue;
			}
			ir->cip->age();

			if (lifetime && ir->cip->lifetime().isValid()) {
				if (!lifetime->isValid() || ir->cip->lifetime() < *lifetime) {
					*lifetime = ir->cip->lifetime();
				}
			}
		}
		it++;
	}

	while (!children.empty()) {
		removed += remove_children(children.pop(), ifl);
	}

	return removed;
}

void InterfaceStore::print()
{
        Mutex::AutoLocker l(mutex);
	InterfaceStore::iterator it = begin();

	printf("====== Interfaces ======\n");
	printf("<type> <identifier> <name> <flags> <age> <refcount> <parent interface>\n");
	printf("\t<addresses>\n");
	printf("------------------------------\n");

	while (it != end()) {
		InterfaceRecord *ir = *it;

		ir->iface.lock();
		InterfaceRef iface = ir->iface;

                printf("%s %s %s %s %s %lu %s\n",
                       iface->getTypeStr(),
                       iface->getIdentifierStr(),
                       iface->getName(),
		       iface->getFlagsStr(),
                       ir->cip ? ir->cip->ageStr() : "undefined",
                       ir->iface.refcount(),
                       ir->parent ? ir->parent->getIdentifierStr() : "\'no parent\'");

		const Addresses *addrs = iface->getAddresses();

		for (Addresses::const_iterator itt = addrs->begin(); itt != addrs->end(); itt++) {
			const Address *addr = *itt;
                       printf("\t%s\n", addr->getURI());
		}

		ir->iface.unlock();
		it++;
	}

	printf("==============================\n");
}

// CBMEN, HL, Begin
void InterfaceStore::getInterfacesAsMetadata(Metadata *m)
{
	Mutex::AutoLocker l(mutex);
	InterfaceStore::iterator it = begin();

	while (it != end()) {
		InterfaceRecord *ir = *it;

		ir->iface.lock();
		InterfaceRef iface = ir->iface;

		Metadata *dm = m->addMetadata("Interface");

		if (!dm)
			continue;

		dm->setParameter("type", iface->getTypeStr());
		dm->setParameter("identifier", iface->getIdentifierStr());
		dm->setParameter("name", iface->getName());
		dm->setParameter("flags", iface->getFlagsStr());
		dm->setParameter("age", ir->cip? ir->cip->ageStr() : "undefined");
		dm->setParameter("ref_count", ir->iface.refcount());
		dm->setParameter("parent", ir->parent ? ir->parent->getIdentifierStr() : "\'no parent\'");

		const Addresses *addrs = iface->getAddresses();
		for (Addresses::const_iterator itt = addrs->begin(); itt != addrs->end(); itt++) {
			const Address *addr = *itt;
			Metadata *dmm = dm->addMetadata("Address");

			if (!dmm)
				continue;

			dmm->setParameter("uri", addr->getURI());
		}

		ir->iface.unlock();
		it++;
	}
}
// CBMEN, HL, End
