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
#ifndef _QUEUE_H_
#define _QUEUE_H_

/*
	Forward declarations of all data types declared in this file. This is to
	avoid circular dependencies. If/when a data type is added to this file,
	remember to add it here.
*/
class QueueElement;
class Queue;

/*
	The data types that can be put into a haggle queue
*/
#include <libcpphaggle/GenericQueue.h>

#include "DataObject.h"
#include "Node.h"
#include "Interface.h"

/**
	The different kinds of data that a QueueElement can contain.
*/
typedef enum {
	// Data Object
	QE_DataObject,
	// No data
	QE_empty
} QEType_t;

/**
	Container object for Queues. Is intended to make the Queue
	insertion/retreival interface less complicated.
*/

class QueueElement
{
	QEType_t type;
	DataObjectRef dObj;
	NodeRef node;
	InterfaceRef iface;
public:
	/**
		This returns the type of data contained.
	*/
	QEType_t getType();

	/**
		The get* functions return NULL if the type isn't right.
	*/
	DataObjectRef getDataObject();

	//	Constructors/deconstructors:
	QueueElement(const DataObjectRef& dObj, const NodeRef targ = NULL, const InterfaceRef iface = NULL);
	QueueElement(const QueueElement &qe);
	~QueueElement();

	friend bool operator==(const QueueElement& q1, const QueueElement& q2);
};

/**
*/
class Queue : public GenericQueue<QueueElement *> {
public:
	Queue(const string _name = "Unnamed Queue") :
		GenericQueue<QueueElement *>(_name) {};
	~Queue() {};

#ifdef DEBUG
	void print();
#endif
};

#endif
