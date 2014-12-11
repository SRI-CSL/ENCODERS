/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

#ifndef FRAGMENTATIONDECODERSTORAGE_H_
#define FRAGMENTATIONDECODERSTORAGE_H_

#include <libcpphaggle/Map.h>
#include <libcpphaggle/String.h>

#include "HaggleKernel.h"
#include "DataObject.h"
#include "DataStore.h"

#include "dataobject/DataObjectTypeIdentifierUtility.h"
#include "fragmentation/utility/FragmentationDataObjectUtility.h"
#include "time/TimeStampUtility.h"

struct FragmentDecoderStruct {
	/*
	 * location storing the dataobject as it is assembled
	 */
	string filePath;
	/*
	 * what the size of the dataobject should be
	 */
	size_t fileSize;
	size_t segmentsRemaining;
};

typedef Reference<FragmentDecoderStruct> FragmentDecoderStructRef;
typedef Map<string, FragmentDecoderStructRef> fragmentdecoderstorage_t;
typedef Reference<fragmentdecoderstorage_t> fragmentdecoderstorage_ref_t;

typedef Map<size_t,bool> sequencenumbertracking_t;
typedef Map<string,sequencenumbertracking_t > dataobjectsequencenumbertracking_t;
typedef Reference<dataobjectsequencenumbertracking_t> dataobjectsequencenumbertracking_ref_t;

typedef Reference<List<string> > fragmentsidlistref;
typedef Map<string,fragmentsidlistref> fragments_t;
typedef Reference<fragments_t> fragments_ref_t;

typedef Map<string,string> reconstructedDataObjectTracker_t;
typedef Reference<reconstructedDataObjectTracker_t> reconstructedDataObjectTracker_ref_t;

//dataobjectid, timestamp
typedef Map<string,string> fragmentationDecoderLastUsedTime_t;
typedef Reference<fragmentationDecoderLastUsedTime_t> fragmentationDecoderLastUsedTime_ref_t;

class FragmentationDecoderStorage {
public:
	FragmentationDecoderStorage(HaggleKernel* _haggleKernel, DataStore* _dataStore,
			DataObjectTypeIdentifierUtility* _dataObjectTypeIdentifierUtility);
	virtual ~FragmentationDecoderStorage();

	bool addFragment(DataObjectRef fragmentDataObject, size_t fragmentSize);

	bool isCompleted(string dataObjectid);

	FragmentDecoderStructRef getFragmentDecoderStruct(string dataObjectId);

	void disposeDataObject(const unsigned char* dataObjectId);

	void disposeFragment(DataObjectRef fragmentDataObjectRef);
	void disposeAssociatedFragments(const string originalDataObjectId);

	fragmentsidlistref getFragments(string originalDataObjectId);
	void addCodedBlockId(string originalDataObjectId,
			const string fragmentDataObjectId);
	void trackFragment(const string originalDataObjectId,
			const string fragmentDataObjectId);

	void trackReconstructedDataObject(const string dataObjectId);

	double getResendReconstructedDelay(const string dataObjectId,double resendDelay);

    void deleteFromStorage(DataObjectRef parentDataObjectRef);

    void trackDecoderLastUsedTime(string dataObjectId);
    void ageOffDecoder(double maxAgeDecoder);
    void deleteDecoderFromStorageByDataObjectId(string parentDataObjectId);
    void cleanUpPartialReconstructedObject(string parentDataObjectId);

    void convertFragmentsToNetworkCodedBlocks(string parentDataObjectId);
    void onRetrievedFragmentFromDataStore(Event* event);

private:
	DataStore* dataStore;
	HaggleKernel* haggleKernel;

	fragmentdecoderstorage_ref_t fragmentDecoderStorage;
	dataobjectsequencenumbertracking_ref_t dataobjectsequencenumbertracking;
	reconstructedDataObjectTracker_ref_t reconstructedDataObjectTracker;

	FragmentationDataObjectUtility* fragmentationDataObjectUtility;
	TimeStampUtility* timeStampUtility;
	DataObjectTypeIdentifierUtility* dataObjectTypeIdentifierUtility;

	fragments_ref_t fragmentsTracker;

	fragmentationDecoderLastUsedTime_ref_t fragmentationDecoderLastUsedTime;
};

#endif /* FRAGMENTATIONDECODERSTORAGE_H_ */
