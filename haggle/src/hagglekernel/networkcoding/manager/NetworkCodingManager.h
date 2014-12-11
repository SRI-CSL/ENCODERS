/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 *   Hasnain Lakhani (HL)
 */

#ifndef NETWORKCODINGMANAGER_H_
#define NETWORKCODINGMANAGER_H_

#include <stdio.h>
#include <stdlib.h>

#include "Manager.h"
#include "ManagerModule.h"
#include "DataObject.h"
#include "Event.h"
#include "networkcoding/databitobject/NetworkCodingDataObjectUtility.h"
#include "networkcoding/NetworkCodingFileUtility.h"
#include "NetworkCodingConfiguration.h"
#include "NetworkCodingSendSuccessFailureEventHandler.h"
#include "networkcoding/service/NetworkCodingDecoderService.h"
#include "networkcoding/managermodule/INetworkCodingDecoderManagerModule.h"
#include "networkcoding/storage/NetworkCodingEncoderStorage.h"
#include "networkcoding/CodeTorrentUtility.h"
#include "networkcoding/managermodule/encoder/INetworkCodingEncoderManagerModule.h"
#include "networkcoding/service/NetworkCodingEncoderService.h"
#include "fragmentation/configuration/FragmentationConfiguration.h"

class NetworkCodingManager: public Manager {
    void onConfig(Metadata *m);

public:
    NetworkCodingManager(HaggleKernel *_haggle);
    virtual ~NetworkCodingManager();

    void onDataObjectForSendNetworkCodedBlock(Event *e);
    void onDataObjectForReceiveNetworkCodedBlock(Event *e);
    void onDataObjectSendSuccessful(Event* e);
    void onDataObjectSendNetworkCodingSuccessful(Event* e);
    void onDataObjectSendFailure(Event* e);
    void onDeleteNetworkCodedBlock(Event* e);
    void onDeleteAssociatedNetworkCodedBlocks(Event* e);
    void onDeletedDataObject(Event *e); // CBMEN, HL
    void onDataObjectAgingNetworkCodedBlock(Event* e);
    void onAgingDecoderNetworkCoding(Event* e);
    void onDisableNetworkCoding(Event* e);
    void onEnableNetworkCoding(Event* e);
    void onIncomingFragmentationToNetworkCodingConversion(Event* e);
    void onNodeQueryResult(Event *e);

    bool init_derived();

    virtual void onShutdown();
private:
    bool processOriginalDataObject(DataObjectRef& dataObjectRef,
            NodeRefList nodeRefList);

    NetworkCodingSendSuccessFailureEventHandler* networkCodingSendSuccessFailureEventHandler;

    NetworkCodingDecoderService* networkCodingDecoderService;
    INetworkCodingDecoderManagerModule* networkCodingDecoderManagerModule;

    /*
     * network coding encoder
     */
    NetworkCodingDataObjectUtility* networkCodingDataObjectUtility;
    NetworkCodingConfiguration* networkCodingConfiguration;
    NetworkCodingDecoderStorage* networkCodingDecoderStorage;
    NetworkCodingFileUtility* networkCodingFileUtility;

    /*
     * network coding encoder
     */
    NetworkCodingEncoderStorage* networkCodingEncoderStorage;
    CodeTorrentUtility* codeTorrentUtility;

    INetworkCodingEncoderManagerModule* networkCodingEncoderManagerModule;
    NetworkCodingEncoderService* networkCodingEncoderService;

    FragmentationConfiguration* fragmentationConfiguration;

    // CBMEN, HL, Begin
    double minTimeBetweenToggles;
    HashMap<string, Timeval> lastToggleTimes;
    EventCallback<EventHandler> *nodeQueryCallback;
    // CBMEN, HL, End
};

#endif /* NETWORKCODINGMANAGER_H_ */
