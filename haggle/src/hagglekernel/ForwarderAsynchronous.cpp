/*
 * This file has been modified from its original version which is part of Haggle 0.4 (2/15/2012).
 * The following notice applies to all these modifications.
 *
 * Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Sam Wood (SW)
 *   Mark-Oliver Stehr (MOS)
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

#include "ForwarderAsynchronous.h"

ForwarderAsynchronous::ForwarderAsynchronous(ForwardingManager *m, const EventType type, const string name, GenericQueue<ForwardingTask *> *taskQ):
	Forwarder(m, name), kernel(getManager()->getKernel()), eventType(type), taskQ(taskQ)
{
}

ForwarderAsynchronous::~ForwarderAsynchronous()
{
    stop();
    
    if (taskQ) {
        delete taskQ;
    }
}

void ForwarderAsynchronous::quit() 
{
    quit(NULL);
}

void ForwarderAsynchronous::quit(RepositoryEntryList *rl)
{
	if (isRunning()) {
		// Tell the thread to quit:
        ForwardingTask *task = new ForwardingTask(NULL, FWD_TASK_QUIT);
        if (NULL != rl) {
            task->setRepositoryEntryList(rl);
        }
		taskQ->insert(task);
		// Make sure noone else adds stuff to the queue:
		taskQ->close();
		// Wait until the thread finishes
		join();
	}
}

void ForwarderAsynchronous::_interfaceQuitHook(const EventType type, ForwardingTask *task, ForwarderAsynchronousInterface *f) 
{
    task->getForwarder()->getSaveState(*task->getRepositoryEntryList());
    // FIXME: this is not the most elegant way to do this, but essentially 
    // during shutdown we pass around the repository entry list to all
    // the forwarders which then append their own state to the list.
    // When we delete the task we don't want to lose the repository entry list.
    task->setRepositoryEntryList(NULL);
    HAGGLE_DBG("Asynchronous Forwarding module %s QUITs\n", task->getForwarder()->getName());
    delete task;
}

void ForwarderAsynchronous::_threadQuitHook(const EventType type, ForwardingTask *task) 
{
    getSaveState(*task->getRepositoryEntryList());
    addEvent(new Event(eventType, task));
    cancel();
}

/*
 General overview of thread loop:
 
 The forwarding module waits for tasks that are input into the task queue (taskQ) by the 
 forwarding manager. Once a task is available, the forwarding module will read it and
 execute any tasks. The module may return results of the task to the forwarding manager
 by using a private event. In that case, it passes the original task object back to the
 manager with any results embedded. Sometimes it might just be enough to signal that a
 a task is complete.
 
 */
bool ForwarderAsynchronous::run(void)
{
  int tasksExecuted = 0; // MOS
  double tasksExecutionTime = 0; // MOS

	while (!shouldExit()) {
		ForwardingTask *task = NULL;
		
		switch (taskQ->retrieve(&task)) {
			case QUEUE_TIMEOUT:
				/*
				 This shouldn't happen - but we make sure the module doesn't
				 break if it does. This means that it has either taken an 
				 exceptionally long time to get the state string, or that it
				 is not coming. Either one is problematic.
				 */
				HAGGLE_DBG("WARNING: timeout occurred in forwarder task queue.\n");
				break;
			case QUEUE_ELEMENT:

			        {
			        // MOS
			        Timeval startTime = Timeval::now();

				switch (task->getType()) {
					case FWD_TASK_NEW_ROUTING_INFO:
                        task->getForwarder()->newRoutingInformation(task->getForwarder()->getRoutingInformation(task->getDataObject()));
						break;
					case FWD_TASK_NEW_NEIGHBOR:
						task->getForwarder()->_newNeighbor(task->getNode());
						break;
					case FWD_TASK_END_NEIGHBOR:
						task->getForwarder()->_endNeighbor(task->getNode());
						break;
					case FWD_TASK_GENERATE_TARGETS:
						task->getForwarder()->_generateTargetsFor(task->getNode());
						break;
						
					case FWD_TASK_GENERATE_DELEGATES:
						task->getForwarder()->_generateDelegatesFor(task->getDataObject(), task->getNode(), task->getNodeList());
						break;
					case FWD_TASK_GENERATE_ROUTING_INFO_DATA_OBJECT:
					        // MOS - do not send to unnamed nodes
					        if (task->getNode()->isNeighbor() && task->getNode()->getType() == Node::TYPE_PEER) {
						  task->setDataObject(task->getForwarder()->createRoutingInformationDataObject());
						  addEvent(new Event(eventType, task));
						}
						task = NULL;
						break;
#ifdef DEBUG
					case FWD_TASK_PRINT_RIB:
						task->getForwarder()->_printRoutingTable();
						break;
#endif
					case FWD_TASK_CONFIG:
						task->getForwarder()->_onForwarderConfig(*task->getConfig());
						break;
					case FWD_TASK_QUIT:
						/*
							When the forwarding module is asked to quit,
							the forwarding manager should save any of its
							state. Return this state to the manager in a 
							callback.
						 */
                        if (NULL == task->getRepositoryEntryList()) {
                            task->setRepositoryEntryList(new RepositoryEntryList());
                        }
                        if (NULL == task->getForwarder()) {
                            _threadQuitHook(eventType, task);
                        } else {
                            _interfaceQuitHook(eventType, task, task->getForwarder());
                        }
                        task = NULL;
						break;
				}

				// MOS
				Timeval endTime = Timeval::now();
				tasksExecuted += 1;
				tasksExecutionTime += (endTime - startTime).getTimeAsSecondsDouble();
                                }

				break;
			default:
				break;
		}
		if (task)
			delete task;
	}

	HAGGLE_STAT("Summary Statistics - Tasks Executed: %d - Avg. Execution Time: %.6f\n", tasksExecuted, tasksExecutionTime/(double)tasksExecuted); // MOS

	return false;
}
