/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */


#ifndef FRAGMENTATIONMANAGER_H_
#define FRAGMENTATIONMANAGER_H_

#include "Manager.h"
#include "ManagerModule.h"
#include "DataObject.h"
#include "Event.h"

#include "dataobject/DataObjectTypeIdentifierUtility.h"
#include "fragmentation/configuration/FragmentationConfiguration.h"
#include "fragmentation/service/FragmentationEncoderService.h"
#include "fragmentation/service/FragmentationDecoderService.h"
#include "fragmentation/utility/FragmentationDataObjectUtility.h"
#include "fragmentation/managermodule/encoder/FragmentationEncoderAsynchronousManagerModule.h"
#include "fragmentation/managermodule/decoder/FragmentationDecoderAsynchronousManagerModule.h"
#include "fragmentation/storage/FragmentationEncoderStorage.h"
#include "fragmentation/manager/FragmentationSendSuccessFailureHandler.h"

class FragmentationEncoderAsynchronousManagerModule;
class FragmentationDecoderAsynchronousManagerModule;

class FragmentationManager: public Manager {
    void onConfig(Metadata *m);

public:
    FragmentationManager(HaggleKernel *_haggle);
    virtual ~FragmentationManager();

    void onDataObjectSendFragmentation(Event *e);
    void onDataObjectForReceived(Event* e);
    void onDataObjectSendSuccessful(Event *e);
    void onDataObjectSendFailure(Event *e);
    void onDeleteFragment(Event* e);
    void onDeleteAssociatedFragments(Event* e);
    void onDeletedDataObject(Event* e);
    void onDataObjectAgingFragmentation(Event* e);
    void onDecoderAgingFragmentation(Event* e);
    void onConvertFragmentationToNetworkcoding(Event* e);

    bool init_derived();

    virtual void onShutdown();

private:
    FragmentationConfiguration* fragmentationConfiguration;
    FragmentationFileUtility* fragmentationFileUtility;
    FragmentationDataObjectUtility* fragmentationDataObjectUtility;
    FragmentationEncoderService* fragmentationEncoderService;
    FragmentationDecoderService* fragmentationDecoderService;
    FragmentationDecoderStorage* fragmentationDecoderStorage;
    FragmentationEncoderStorage* fragmentationEncoderStorage;
    DataObjectTypeIdentifierUtility* dataObjectTypeIdentifierUtility;
    FragmentationEncoderAsynchronousManagerModule* fragmentationEncoderAsynchronousManagerModule;
    FragmentationDecoderAsynchronousManagerModule* fragmentationDecoderAsynchronousManagerModule;
    FragmentationSendSuccessFailureHandler* fragmentationSendSuccessFailureHandler;
};

#endif /* FRAGMENTATIONMANAGER_H_ */
