/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#include "NetworkCodingDecoderManagerModule.h"

NetworkCodingDecoderManagerModule::NetworkCodingDecoderManagerModule(
        NetworkCodingDecoderService* _networkCodingDecoderService,
	NetworkCodingConfiguration* _networkCodingConfiguration,
        NetworkCodingManager* _networkCodingManager, const string name) :
        ManagerModule<NetworkCodingManager>(_networkCodingManager, name) {

    this->networkCodingDecoderService = _networkCodingDecoderService;
    this->networkCodingConfiguration = _networkCodingConfiguration;
    this->networkCodingDecoderManagerModuleProcessor =
            new NetworkCodingDecoderManagerModuleProcessor(
	      this->networkCodingDecoderService, this->networkCodingConfiguration, this->getKernel());

}

NetworkCodingDecoderManagerModule::~NetworkCodingDecoderManagerModule() {
    if(this->networkCodingDecoderManagerModuleProcessor) {
        delete this->networkCodingDecoderManagerModuleProcessor;
        this->networkCodingDecoderManagerModuleProcessor = NULL;
    }
}

bool NetworkCodingDecoderManagerModule::addTask(
        NetworkCodingDecodingTaskRef networkCodingDecodingTask) {

    this->networkCodingDecoderManagerModuleProcessor->decode(networkCodingDecodingTask);

    return true;
}

bool NetworkCodingDecoderManagerModule::startup() {
    return true;
}

bool NetworkCodingDecoderManagerModule::shutdownAndClose() {
    this->stop();
    return true;
}
