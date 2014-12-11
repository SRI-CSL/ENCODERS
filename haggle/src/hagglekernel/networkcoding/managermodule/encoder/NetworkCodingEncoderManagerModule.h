/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef NETWORKCODINGENCODERMANAGERMODULE_H_
#define NETWORKCODINGENCODERMANAGERMODULE_H_

//forward declaration
class NetworkCodingEncoderManagerModule;
class NetworkCodingManager;

#include "ManagerModule.h"
#include "networkcoding/manager/NetworkCodingManager.h"
#include "networkcoding/service/NetworkCodingEncoderService.h"
#include "networkcoding/concurrent/encoder/NetworkCodingEncoderTask.h"
#include "networkcoding/managermodule/encoder/INetworkCodingEncoderManagerModule.h"
#include "networkcoding/managermodule/encoder/NetworkCodingEncoderManagerModuleProcessor.h"

class NetworkCodingEncoderManagerModule: public ManagerModule<NetworkCodingManager>, public INetworkCodingEncoderManagerModule {
public:
    NetworkCodingEncoderManagerModule(NetworkCodingEncoderService* _networkCodingEncoderService,
            NetworkCodingManager* _networkCodingManager, const string name);
    virtual ~NetworkCodingEncoderManagerModule();

    bool addTask(NetworkCodingEncoderTaskRef networkCodingEncoderTask);

    bool startup();

    bool shutdownAndClose();
private:
    NetworkCodingEncoderService* networkCodingEncoderService;
    NetworkCodingEncoderManagerModuleProcessor* networkCodingEncoderManagerModuleProcessor;
};

#endif /* NETWORKCODINGENCODERMANAGERMODULE_H_ */
