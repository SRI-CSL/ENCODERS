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
#include <libcpphaggle/Platform.h>
#include <libcpphaggle/Watch.h>

#include "Queue.h"

/*
	QueueElement class implementation.
*/

QEType_t QueueElement::getType()
{
	return type;
}

DataObjectRef QueueElement::getDataObject()
{
	return dObj;
}

/*
	Constructor: DataObject. Insert data object.
*/
QueueElement::QueueElement(const DataObjectRef& _obj, const NodeRef _node, const InterfaceRef _iface) : 
	type(QE_DataObject), 
	dObj(_obj),
	node(_node),
	iface(_iface)
	
{
}

QueueElement::QueueElement(const QueueElement &qe) : 
	type(qe.type),
	dObj(qe.dObj),
	node(qe.node),
	iface(qe.iface)
{
}

bool operator==(const QueueElement& q1, const QueueElement& q2)
{
	if (q1.type == QE_DataObject && q2.type == QE_DataObject) {
		return (q1.dObj == q2.dObj && q1.node == q2.node && q1.iface == q2.iface);
	}
	return false;
}
/*
	Destructor: does nothing
*/
QueueElement::~QueueElement(void)
{
}

#ifdef DEBUG
void Queue::print()
{
        Mutex::AutoLocker l(mutex);
	List<QueueElement *>::iterator it;
	unsigned long n = 0;

	for (it = lst.begin(); it != lst.end(); it++) {
		QueueElement *qe = *it;
		char *raw = NULL;
		size_t len;

		switch(qe->getType()) {
		case QE_DataObject:
			qe->getDataObject()->getRawMetadataAlloc((unsigned char **)&raw, &len);
			if (raw && len > 0) {
				printf("DataObject metadata: %lu\n", n);
				printf("--------------------\n%s\n", raw);
                                printf("--------------------\n");
				free(raw);
			}
			break;
		case QE_empty:
		default:
			break;
		}
	}
}
#endif /* DEBUG */

