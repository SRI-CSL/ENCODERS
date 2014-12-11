/* Copyright (c) 2014 SRI International and Suns-tech Incorporated and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 *   Sam Wood (SW)
 */

#include "NetworkCodingEncoderAsynchronousManagerModule.h"

#include "Event.h"
#include "NetworkCodingEncoderManagerModule.h"
#include "INetworkCodingEncoderManagerModule.h"

NetworkCodingEncoderAsynchronousManagerModule::NetworkCodingEncoderAsynchronousManagerModule(
        NetworkCodingEncoderService* _networkCodingEncoderService,
        NetworkCodingManager* _networkCodingManager, const string name) :
        ManagerModule<NetworkCodingManager>(_networkCodingManager, name) {

    // SW
    minEncoderDelayBase = 0;
    minEncoderDelayLinear = 0;
    minEncoderDelaySquare = 0;

    this->networkCodingEncoderService = _networkCodingEncoderService;
    this->networkCodingEncoderManagerModuleProcessor =
            new NetworkCodingEncoderManagerModuleProcessor(
                    this->networkCodingEncoderService, this->getKernel());
}

NetworkCodingEncoderAsynchronousManagerModule::~NetworkCodingEncoderAsynchronousManagerModule() {
    if (this->networkCodingEncoderManagerModuleProcessor) {
        delete this->networkCodingEncoderManagerModuleProcessor;
        this->networkCodingEncoderManagerModuleProcessor = NULL;
    }
}

bool NetworkCodingEncoderAsynchronousManagerModule::addTask(
        NetworkCodingEncoderTaskRef networkCodingDecodingTask) {
    NetworkCodingEncoderTask* task = networkCodingDecodingTask.getObj();
    taskQueue.insert(task);
    //return taskQueue.insert(NULL);
    return true;
}

void NetworkCodingEncoderAsynchronousManagerModule::cleanup() {
    HAGGLE_DBG(
            "calling NetworkCodingEncoderAsynchronousManagerModule cleanup\n");
    this->taskQueue.close();
    this->stop();
}

bool NetworkCodingEncoderAsynchronousManagerModule::run() {
    HAGGLE_DBG("NetworkCodingEncoderAsynchronousManagerModule running...\n");

    while (!shouldExit()) {
        QueueEvent_t qe;
        NetworkCodingEncoderTaskRef task = NULL;

        qe = taskQueue.retrieve(&task);

        switch (qe) {
        case QUEUE_ELEMENT:
            doTask(task);

            // Delete task here or return it with result in private event?
            //delete task;
            break;
        case QUEUE_WATCH_ABANDONED:
            HAGGLE_DBG(
                    "NetworkCodingEncoderAsynchronousManagerModule instructed to exit...\n");
            return false;
        default:
            HAGGLE_ERR(
                    "Unknown NetworkCodingEncoderTask task queue return value\n");
            break;
        }

	// MOS - basic encoder rate limit
	int minEncoderDelay = minEncoderDelayBase;
	int numNeighbors = getKernel()->getNodeStore()->numNeighbors();
	minEncoderDelay += minEncoderDelayLinear * numNeighbors;
	minEncoderDelay += minEncoderDelaySquare * numNeighbors * numNeighbors;
	cancelableSleep(minEncoderDelay);
    }
    return false;
}

void NetworkCodingEncoderAsynchronousManagerModule::doTask(
        NetworkCodingEncoderTaskRef task) {

    switch (task->getType()) {
    case NETWORKCODING_ENCODING_TASK_ENCODE:
        HAGGLE_DBG2("NETWORKCODING_ENCODING_TASK_ENCODE\n");

        this->encode(task);
        break;

    }

    return;
}

void NetworkCodingEncoderAsynchronousManagerModule::encode(
        NetworkCodingEncoderTaskRef networkCodingEncodingTask) {

    this->networkCodingEncoderManagerModuleProcessor->encode(
            networkCodingEncodingTask);
}

bool NetworkCodingEncoderAsynchronousManagerModule::startup() {
    return this->start();
}

bool NetworkCodingEncoderAsynchronousManagerModule::shutdownAndClose() {
    this->cleanup();
    return true;
}
