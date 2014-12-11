/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef NETWORKCODINGDECODERMANAGERMODULE_H_
#define NETWORKCODINGDECODERMANAGERMODULE_H_

#include "Event.h"
#include "ManagerModule.h"
#include "networkcoding/manager/NetworkCodingManager.h"
#include "networkcoding/service/NetworkCodingDecoderService.h"
#include "networkcoding/concurrent/NetworkCodingDecodingTask.h"
#include "networkcoding/managermodule/decoder/NetworkCodingDecoderManagerModuleProcessor.h"

class NetworkCodingDecoderManagerModule: public ManagerModule<NetworkCodingManager>, public INetworkCodingDecoderManagerModule {
public:
    NetworkCodingDecoderManagerModule(
            NetworkCodingDecoderService* _networkCodingDecoderService,
	    NetworkCodingConfiguration* _networkCodingConfiguration,
            NetworkCodingManager* _networkCodingManager, const string name);
    virtual ~NetworkCodingDecoderManagerModule();

    bool addTask(NetworkCodingDecodingTaskRef networkCodingDecodingTask);

    bool startup();
    bool shutdownAndClose();
private:
    NetworkCodingDecoderService* networkCodingDecoderService;
    NetworkCodingConfiguration* networkCodingConfiguration;
    NetworkCodingDecoderManagerModuleProcessor* networkCodingDecoderManagerModuleProcessor;

};

#endif /* NETWORKCODINGDECODERMANAGERMODULE_H_ */
