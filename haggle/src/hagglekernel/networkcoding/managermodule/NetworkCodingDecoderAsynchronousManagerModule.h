/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef NETWORKCODINGDECODERASYNCHRONOUSMANAGERMODULE_H_
#define NETWORKCODINGDECODERASYNCHRONOUSMANAGERMODULE_H_

#include <libcpphaggle/GenericQueue.h>

#include "Event.h"
#include "ManagerModule.h"
#include "networkcoding/manager/NetworkCodingManager.h"
#include "networkcoding/service/NetworkCodingDecoderService.h"
#include "networkcoding/concurrent/NetworkCodingDecodingTask.h"
#include "networkcoding/managermodule/decoder/NetworkCodingDecoderManagerModuleProcessor.h"

class NetworkCodingDecoderAsynchronousManagerModule: public ManagerModule<
        NetworkCodingManager>, public INetworkCodingDecoderManagerModule {
public:
    NetworkCodingDecoderAsynchronousManagerModule(
            NetworkCodingDecoderService* _networkCodingDecoderService,
	    NetworkCodingConfiguration* _networkCodingConfiguration,
            NetworkCodingManager* _networkCodingManager, const string name);
    virtual ~NetworkCodingDecoderAsynchronousManagerModule();

    bool addTask(NetworkCodingDecodingTaskRef networkCodingDecodingTask);

    bool run();
    void cleanup();

    bool startup();

    bool shutdownAndClose();
private:
    void doTask(NetworkCodingDecodingTaskRef task);
    void decode(NetworkCodingDecodingTaskRef task);

    NetworkCodingDecoderService* networkCodingDecoderService;
    NetworkCodingConfiguration* networkCodingConfiguration;
    GenericQueue<NetworkCodingDecodingTaskRef> taskQueue;

    NetworkCodingDecoderManagerModuleProcessor* networkCodingDecoderManagerModuleProcessor;
};

#endif /* NETWORKCODINGDECODERASYNCHRONOUSMANAGERMODULE_H_ */
