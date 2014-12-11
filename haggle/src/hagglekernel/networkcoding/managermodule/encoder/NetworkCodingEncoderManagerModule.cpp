/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "Event.h"
#include "NetworkCodingEncoderManagerModule.h"
#include "INetworkCodingEncoderManagerModule.h"

NetworkCodingEncoderManagerModule::NetworkCodingEncoderManagerModule(
        NetworkCodingEncoderService* _networkCodingEncoderService,
        NetworkCodingManager* _networkCodingManager, const string name) :
        ManagerModule<NetworkCodingManager>(_networkCodingManager, name) {

    // MOS
    minEncoderDelayBase = 0;
    minEncoderDelayLinear = 0;
    minEncoderDelaySquare = 0;

    this->networkCodingEncoderService = _networkCodingEncoderService;
    this->networkCodingEncoderManagerModuleProcessor =
            new NetworkCodingEncoderManagerModuleProcessor(
                    this->networkCodingEncoderService, this->getKernel());
}

NetworkCodingEncoderManagerModule::~NetworkCodingEncoderManagerModule() {
    if (this->networkCodingEncoderManagerModuleProcessor) {
        delete this->networkCodingEncoderManagerModuleProcessor;
        this->networkCodingEncoderManagerModuleProcessor = NULL;
    }
}

bool NetworkCodingEncoderManagerModule::addTask(
        NetworkCodingEncoderTaskRef networkCodingEncoderTask) {

    this->networkCodingEncoderManagerModuleProcessor->encode(
            networkCodingEncoderTask);

    return true;
}

bool NetworkCodingEncoderManagerModule::startup() {
    return this->start();
}

bool NetworkCodingEncoderManagerModule::shutdownAndClose() {
    this->stop();
    return true;
}
