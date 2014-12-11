/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Mark-Oliver Stehr (MOS)
 */

#ifndef FRAGMENTATIONENCODERASYNCHRONOUSMANAGERMODULE_H_
#define FRAGMENTATIONENCODERASYNCHRONOUSMANAGERMODULE_H_

#include <libcpphaggle/GenericQueue.h>

#include "ManagerModule.h"
#include "fragmentation/manager/FragmentationManager.h"
#include "fragmentation/service/FragmentationEncoderService.h"
#include "fragmentation/concurrent/encoder/FragmentationEncodingTask.h"
#include "fragmentation/configuration/FragmentationConfiguration.h"
#include "fragmentation/utility/FragmentationDataObjectUtility.h"

class FragmentationManager;

class FragmentationEncoderAsynchronousManagerModule: public ManagerModule<FragmentationManager> {
public:
    int minEncoderDelayBase, minEncoderDelayLinear, minEncoderDelaySquare; // MOS

	FragmentationEncoderAsynchronousManagerModule(
			FragmentationConfiguration* _fragmentationConfiguration,
			FragmentationDataObjectUtility* _fragmentationDataObjectUtility,
			FragmentationEncoderService* _fragmentationEncoderService,
			FragmentationManager* _fragmentationManager, const string name);
    virtual ~FragmentationEncoderAsynchronousManagerModule();

    bool addTask(FragmentationEncodingTaskRef fragmentationEncodingTask);

    bool startup();

    bool run();
    void cleanup();

    bool shutdownAndClose();
private:
    void doTask(FragmentationEncodingTaskRef task);
    void encode(FragmentationEncodingTaskRef task);

    GenericQueue<FragmentationEncodingTaskRef> taskQueue;

    FragmentationEncoderService* fragmentationEncoderService;
    FragmentationDataObjectUtility* fragmentationDataObjectUtility;
    FragmentationConfiguration* fragmentationConfiguration;


};

#endif /* FRAGMENTATIONENCODERASYNCHRONOUSMANAGERMODULE_H_ */
