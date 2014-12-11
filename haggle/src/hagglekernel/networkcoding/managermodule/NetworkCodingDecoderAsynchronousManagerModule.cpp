/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "NetworkCodingDecoderAsynchronousManagerModule.h"
#include "Trace.h"

NetworkCodingDecoderAsynchronousManagerModule::NetworkCodingDecoderAsynchronousManagerModule(
        NetworkCodingDecoderService* _networkCodingDecoderService,
        NetworkCodingConfiguration* _networkCodingConfiguration,
        NetworkCodingManager* _networkCodingManager, const string name) :
        ManagerModule<NetworkCodingManager>(_networkCodingManager, name), taskQueue(
                "NetworkCodingDecoderAsynchronousManagerModule") {

    this->networkCodingDecoderService = _networkCodingDecoderService;
    this->networkCodingConfiguration = _networkCodingConfiguration;
    this->networkCodingDecoderManagerModuleProcessor =
            new NetworkCodingDecoderManagerModuleProcessor(
  	      this->networkCodingDecoderService, this->networkCodingConfiguration, this->getKernel());

}

NetworkCodingDecoderAsynchronousManagerModule::~NetworkCodingDecoderAsynchronousManagerModule() {
    if (this->networkCodingDecoderManagerModuleProcessor) {
        delete this->networkCodingDecoderManagerModuleProcessor;
        this->networkCodingDecoderManagerModuleProcessor = NULL;
    }
}

bool NetworkCodingDecoderAsynchronousManagerModule::addTask(
        NetworkCodingDecodingTaskRef networkCodingDecodingTask) {
    return taskQueue.insert(networkCodingDecodingTask);
}

void NetworkCodingDecoderAsynchronousManagerModule::cleanup() {
    taskQueue.close();
    this->stop();
}

bool NetworkCodingDecoderAsynchronousManagerModule::run() {
    HAGGLE_DBG("NetworkCodingDecoderAsynchronousManagerModule running...\n");

    while (!shouldExit()) {
        QueueEvent_t qe;
        NetworkCodingDecodingTaskRef task = NULL;

        qe = taskQueue.retrieve(&task);

        switch (qe) {
        case QUEUE_ELEMENT:
            doTask(task);

            // Delete task here or return it with result in private event?
            //delete task;
            break;
        case QUEUE_WATCH_ABANDONED:
            HAGGLE_DBG(
                    "NetworkCodingDecoderAsynchronousManagerModule instructed to exit...\n");
            return false;
        default:
            HAGGLE_ERR(
                    "Unknown NetworkCodingDecodingTask task queue return value\n");
            break;
        }
    }
    return false;
}

void NetworkCodingDecoderAsynchronousManagerModule::doTask(
        NetworkCodingDecodingTaskRef task) {

    switch (task->getType()) {
    case NETWORKCODING_DECODING_TASK_DECODE:
        HAGGLE_DBG2("NETWORKCODING_DECODING_TASK_DECODE\n");

        this->decode(task);
        break;

    }

    return;
}

void NetworkCodingDecoderAsynchronousManagerModule::decode(
        NetworkCodingDecodingTaskRef networkCodingDecodingTask) {

    this->networkCodingDecoderManagerModuleProcessor->decode(
            networkCodingDecodingTask);
}

bool NetworkCodingDecoderAsynchronousManagerModule::startup() {
    return this->start();
}

bool NetworkCodingDecoderAsynchronousManagerModule::shutdownAndClose() {
    this->cleanup();
    return true;
}
