/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef NETWORKCODINGENCODERASYNCHRONOUSMANAGERMODULE_H_
#define NETWORKCODINGENCODERASYNCHRONOUSMANAGERMODULE_H_

#include <libcpphaggle/GenericQueue.h>

#include "ManagerModule.h"
#include "networkcoding/manager/NetworkCodingManager.h"
#include "networkcoding/service/NetworkCodingEncoderService.h"
#include "networkcoding/concurrent/encoder/NetworkCodingEncoderTask.h"
#include "networkcoding/managermodule/encoder/INetworkCodingEncoderManagerModule.h"
#include "networkcoding/managermodule/encoder/NetworkCodingEncoderManagerModuleProcessor.h"

class NetworkCodingEncoderAsynchronousManagerModule: public ManagerModule<NetworkCodingManager>, public INetworkCodingEncoderManagerModule {
public:
    NetworkCodingEncoderAsynchronousManagerModule(
            NetworkCodingEncoderService* _networkCodingEncoderService,
            NetworkCodingManager* _networkCodingManager, const string name);
    virtual ~NetworkCodingEncoderAsynchronousManagerModule();

    bool addTask(NetworkCodingEncoderTaskRef networkCodingEncoderTask);

    bool startup();

    bool run();
    void cleanup();

    bool shutdownAndClose();
private:
    void doTask(NetworkCodingEncoderTaskRef task);
    void encode(NetworkCodingEncoderTaskRef task);

    NetworkCodingEncoderService* networkCodingEncoderService;
    GenericQueue<NetworkCodingEncoderTaskRef> taskQueue;

    NetworkCodingEncoderManagerModuleProcessor* networkCodingEncoderManagerModuleProcessor;
};

#endif /* NETWORKCODINGENCODERASYNCHRONOUSMANAGERMODULE_H_ */
