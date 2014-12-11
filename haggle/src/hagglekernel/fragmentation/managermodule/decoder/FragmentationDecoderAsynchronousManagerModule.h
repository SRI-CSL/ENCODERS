/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef FRAGMENTATIONDECODERASYNCHRONOUSMANAGERMODULE_H_
#define FRAGMENTATIONDECODERASYNCHRONOUSMANAGERMODULE_H_

#include <libcpphaggle/GenericQueue.h>

#include "Event.h"
#include "ManagerModule.h"
#include "fragmentation/manager/FragmentationManager.h"
#include "fragmentation/service/FragmentationDecoderService.h"
#include "fragmentation/concurrent/decoder/FragmentationDecodingTask.h"
#include "fragmentation/configuration/FragmentationConfiguration.h"

class FragmentationManager;

class FragmentationDecoderAsynchronousManagerModule: public ManagerModule<FragmentationManager>{
public:
	FragmentationDecoderAsynchronousManagerModule(
			FragmentationConfiguration* _fragmentationConfiguration,
			FragmentationDecoderService* _fragmentationDecoderService,
			FragmentationManager* _fragmentationManager, const string name);
    virtual ~FragmentationDecoderAsynchronousManagerModule();

    bool addTask(FragmentationDecodingTaskRef fragmentationDecodingTaskRef);

    bool run();
    void cleanup();

    bool startup();

    bool shutdownAndClose();
private:
    void doTask(FragmentationDecodingTaskRef fragmentationDecodingTaskRef);
    void decode(FragmentationDecodingTaskRef fragmentationDecodingTaskRef);

    FragmentationDecoderService* fragmentationDecoderService;
    FragmentationConfiguration* fragmentationConfiguration;
    GenericQueue<FragmentationDecodingTaskRef> taskQueue;
};

#endif /* FRAGMENTATIONDECODERASYNCHRONOUSMANAGERMODULE_H_ */
