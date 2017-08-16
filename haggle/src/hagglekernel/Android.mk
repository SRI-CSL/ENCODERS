# Copyright 2008 Uppsala University

LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

# Do not build for simulator
ifneq ($(TARGET_SIMULATOR),true)

# Python for Android
PY4A := ../../../ccb/py4a
PY4A_INC := $(PY4A)/Include
PY4A_LIB := $(PY4A)

PY4A_CFLAGS := -I$(PY4A) -I$(PY4A_INC) -fno-strict-aliasing -g -O2 -DNDEBUG -g -fwrapv -O2 -Wall
PY4A_LDFLAGS := -ldl -lm -lpython2.7 -Xlinker -export-dynamic -Wl,-O1 -Wl,-Bsymbolic-functions

#
# Haggle
#
LOCAL_SRC_FILES := \
	../utils/base64.c \
	../utils/bloomfilter.c \
	../utils/counting_bloomfilter.c \
	../utils/utils.c \
	../utils/prng.c \
	Address.cpp \
	ApplicationManager.cpp \
	InterestManager.cpp \
	Attribute.cpp \
	BenchmarkManager.cpp \
	ConnectivityBluetooth.cpp \
	ConnectivityBluetoothLinux.cpp \
	Connectivity.cpp \
	ConnectivityEthernet.cpp \
	ConnectivityInterfacePolicy.cpp \
	ConnectivityLocal.cpp \
	ConnectivityManager.cpp \
	Bloomfilter.cpp \
	Certificate.cpp \
	DataManager.cpp \
	SendPriorityManager.cpp \
	MemoryCache.cpp \
	ReplicationManagerFactory.cpp \
	ReplicationManager.cpp \
	ReplicationManagerModule.cpp \
	ReplicationManagerAsynchronous.cpp \
	ReplicationManagerUtility.cpp \
	ReplicationConnectState.cpp \
	ReplicationNodeDurationAverager.cpp \
	ReplicationSendRateAverager.cpp \
	ReplicationConcurrentSend.cpp \
	ReplicationOptimizer.cpp \
	ReplicationGlobalOptimizerFactory.cpp \
	ReplicationGlobalOptimizer.cpp \
	ReplicationKnapsackOptimizerFactory.cpp \
	ReplicationKnapsackOptimizer.cpp \
	ReplicationUtilityFunctionFactory.cpp \
	ReplicationUtilityFunction.cpp \
	CacheStrategy.cpp \
	CacheStrategyAsynchronous.cpp \
	CacheStrategyUtility.cpp \
	CacheUtilityFunction.cpp \
	CacheUtilityFunctionFactory.cpp \
	CacheKnapsackOptimizer.cpp \
	CacheKnapsackOptimizerFactory.cpp \
	CacheGlobalOptimizer.cpp \
	CacheGlobalOptimizerFactory.cpp \
	CacheStrategyFactory.cpp \
	CacheStrategyReplacementPurger.cpp \
	CacheReplacement.cpp \
	CacheReplacementFactory.cpp \
	CacheReplacementTotalOrder.cpp \
	CacheReplacementPriority.cpp \
	CachePurger.cpp \
	CachePurgerFactory.cpp \
	CachePurgerAbsTTL.cpp \
	CachePurgerRelTTL.cpp \
	CachePurgerParallel.cpp \
	DataObject.cpp \
	DataStore.cpp \
	MemoryDataStore.cpp \
	Debug.cpp \
	DebugManager.cpp \
	Event.cpp \
	Filter.cpp \
	Forwarder.cpp \
	ForwarderFactory.cpp \
	ForwardingClassifier.cpp \
	ForwardingClassifierAllMatch.cpp \
	ForwardingClassifierBasic.cpp \
	ForwardingClassifierNodeDescription.cpp \
	ForwardingClassifierAttribute.cpp \
	ForwardingClassifierPriority.cpp \
	ForwardingClassifierSizeRange.cpp \
	ForwardingClassifierFactory.cpp \
	ForwardingManager.cpp \
	ForwarderAggregate.cpp \
	ForwarderFlood.cpp \
	ForwarderNone.cpp \
	ForwarderAsynchronous.cpp \
	ForwarderAsynchronousInterface.cpp \
	ForwarderProphet.cpp \
	ForwarderAlphaDirect.cpp \
	HaggleKernel.cpp \
	Interface.cpp \
	InterfaceStore.cpp \
	main.cpp \
	Scratchpad.cpp \
	ScratchpadManager.cpp \
	EvictStrategyLRFU.cpp \
	EvictStrategy.cpp \
	EvictStrategyFactory.cpp \
	EvictStrategyManager.cpp \
	EvictStrategyLRU_K.cpp \
	Manager.cpp \
	Node.cpp \
	NodeManager.cpp \
	NodeStore.cpp \
	Policy.cpp \
	SocketWrapper.cpp \
	SocketWrapperTCP.cpp \
	SocketWrapperUDP.cpp \
	Protocol.cpp \
	ProtocolLOCAL.cpp \
	ProtocolManager.cpp \
	ProtocolRFCOMM.cpp \
	ProtocolSocket.cpp \
	ProtocolTCP.cpp \
	ProtocolUDP.cpp \
	ProtocolUDPGeneric.cpp \
	ProtocolUDPBroadcast.cpp \
	ProtocolUDPBroadcastProxy.cpp \
	ProtocolUDPUnicast.cpp \
	ProtocolUDPUnicastProxy.cpp \
	ProtocolClassifier.cpp \
	ProtocolClassifierAttribute.cpp \
	ProtocolClassifierFactory.cpp \
	ProtocolClassifierBasic.cpp \
	ProtocolClassifierNodeDescription.cpp \
	ProtocolClassifierSizeRange.cpp \
	ProtocolClassifierAllMatch.cpp \
	ProtocolClassifierPriority.cpp \
	ProtocolConfiguration.cpp \
	ProtocolConfigurationFactory.cpp \
	ProtocolConfigurationTCP.cpp \
	ProtocolConfigurationUDPGeneric.cpp \
	ProtocolFactory.cpp \
	Queue.cpp \
	RepositoryEntry.cpp \
	ResourceManager.cpp \
	ResourceMonitor.cpp \
	ResourceMonitorAndroid.cpp \
	CharmCryptoBridge.cpp \
	SecurityManager.cpp \
	SQLDataStore.cpp \
	Trace.cpp \
	Utility.cpp \
	Metadata.cpp \
	XMLMetadata.cpp \
	jni.cpp \
        networkcoding/CodeTorrentUtility.cpp \
        networkcoding/databitobject/NetworkCodingDataObjectUtility.cpp \
        networkcoding/NetworkCodingFileUtility.cpp \
        networkcoding/codetorrent.cpp \
        networkcoding/codetorrentdecoder.cpp \
        networkcoding/codetorrentencoder.cpp \
        networkcoding/galois.cpp \
        networkcoding/nc.cpp \
        networkcoding/singleblockencoder.cpp \
        networkcoding/manager/NetworkCodingConfiguration.cpp \
        networkcoding/forwarding/NetworkCodingForwardingManagerHelper.cpp \
        networkcoding/protocol/NetworkCodingProtocolHelper.cpp \
        networkcoding/manager/NetworkCodingManager.cpp \
        networkcoding/manager/NetworkCodingSendSuccessFailureEventHandler.cpp \
        networkcoding/storage/NetworkCodingEncoderStorage.cpp \
        networkcoding/storage/NetworkCodingDecoderStorage.cpp \
        networkcoding/application/ApplicationManagerHelper.cpp \
        networkcoding/service/NetworkCodingDecoderService.cpp \
        networkcoding/managermodule/NetworkCodingDecoderManagerModule.cpp \
        networkcoding/managermodule/NetworkCodingDecoderAsynchronousManagerModule.cpp \
        networkcoding/service/NetworkCodingEncoderService.cpp \
        networkcoding/managermodule/encoder/NetworkCodingEncoderManagerModule.cpp \
        networkcoding/concurrent/encoder/NetworkCodingEncoderTask.cpp \
        networkcoding/managermodule/encoder/NetworkCodingEncoderAsynchronousManagerModule.cpp \
        networkcoding/concurrent/NetworkCodingDecodingTask.cpp \
        networkcoding/managermodule/decoder/NetworkCodingDecoderManagerModuleProcessor.cpp \
        networkcoding/managermodule/encoder/NetworkCodingEncoderManagerModuleProcessor.cpp \
        networkcoding/bloomfilter/NetworkCodingBloomFilterHelper.cpp \
        fragmentation/manager/FragmentationManager.cpp \
        fragmentation/fragment/FragmentationFileUtility.cpp \
        fragmentation/service/FragmentationEncoderService.cpp \
        fragmentation/service/FragmentationDecoderService.cpp \
        fragmentation/storage/FragmentationDecoderStorage.cpp \
        fragmentation/utility/FragmentationDataObjectUtility.cpp \
        fragmentation/protocol/FragmentationProtocolHelper.cpp \
        fragmentation/configuration/FragmentationConfiguration.cpp \
	networkcoding/blocky/src/blockycoderfile.cpp \
	networkcoding/blocky/src/blockycoder.cpp \
	networkcoding/blocky/src/coder.cpp \
	networkcoding/blocky/src/gf28.cpp \
        dataobject/DataObjectTypeIdentifierUtility.cpp \
        time/TimeStampUtility.cpp \
        fragmentation/concurrent/encoder/FragmentationEncodingTask.cpp \
        fragmentation/concurrent/decoder/FragmentationDecodingTask.cpp \
        fragmentation/managermodule/decoder/FragmentationDecoderAsynchronousManagerModule.cpp \
        fragmentation/storage/FragmentationEncoderStorage.cpp \
        fragmentation/manager/FragmentationSendSuccessFailureHandler.cpp \
        fragmentation/forwarding/FragmentationForwardingManagerHelper.cpp \
        fragmentation/bloomfilter/FragmentationBloomFilterHelper.cpp \
        fragmentation/managermodule/encoder/FragmentationEncoderAsynchronousManagerModule.cpp \
        stringutils/CSVUtility.cpp \
        LossEstimateManager.cpp \
        LossRateSlidingWindowElement.cpp \
        LossRateSlidingWindowEstimator.cpp

# Includes for the TI wlan driver API
TI_STA_INCLUDES := \
	system/wlan/ti/sta_dk_4_0_4_32/common/src/hal/FirmwareApi \
	system/wlan/ti/sta_dk_4_0_4_32/common/inc \
	system/wlan/ti/sta_dk_4_0_4_32/common/src/inc \
	system/wlan/ti/sta_dk_4_0_4_32/pform/linux/inc \
	system/wlan/ti/sta_dk_4_0_4_32/CUDK/Inc

LOCAL_C_INCLUDES += \
	$(LOCAL_PATH)/../../extlibs/libxml2-2.9.0/include \
        $(LOCAL_PATH)/../libcpphaggle/include \
        $(LOCAL_PATH)/../utils \
        $(LOCAL_PATH)/../libhaggle/include \
	$(EXTRA_LIBS_PATH)/core/include \
	$(EXTRA_LIBS_PATH)/sqlite/dist \
	$(EXTRA_LIBS_PATH)/openssl \
	$(EXTRA_LIBS_PATH)/openssl/include \
	$(EXTRA_LIBS_PATH)/dbus \
	$(EXTRA_LIBS_PATH)/bluetooth/bluez/libs/include \
	$(EXTRA_LIBS_PATH)/bluetooth/bluez/include \
	$(EXTRA_LIBS_PATH)/bluetooth/bluez/libs \
	$(EXTRA_LIBS_PATH)/bluetooth/bluez/lib \
	$(JNI_H_INCLUDE)

# We need to compile our own version of libxml2, because the static
# library provided in Android does not have the configured options we need.
LOCAL_LDLIBS := \
	-lsqlite \
	-lcrypto \
	-llog

LOCAL_SHARED_LIBRARIES += \
	libdl \
	libstdc++ \
	libsqlite \
	libcrypto \
	liblog

LOCAL_STATIC_LIBRARIES += \
	libcpphaggle \
	libhaggle-xml2

LOCAL_DEFINES:= \
	-DHAVE_CONFIG \
	-DOS_ANDROID \
	-DOS_LINUX \
	-DDEBUG \
	-DHAVE_EXCEPTION=0 \
	-DENABLE_ETHERNET \
#	-DENABLE_BLUETOOTH \
	$(EXTRA_DEFINES)

LOCAL_CFLAGS :=-O2 -g $(LOCAL_DEFINES)
LOCAL_CPPFLAGS +=$(LOCAL_DEFINES)
LOCAL_CPPFLAGS +=-fexceptions
LOCAL_LDFLAGS +=-L$(EXTRA_LIBS_PATH)/../libs/$(TARGET_ARCH_ABI)

# Python for Android
LOCAL_CFLAGS += $(PY4A_CFLAGS)
LOCAL_LDFLAGS += -L$(PY4A_LIB) -L$(PY4A_LIB)/Lib $(PY4A_LDFLAGS)

ifneq ($(BOARD_WLAN_TI_STA_DK_ROOT),)
# For devices with tiwlan driver
LOCAL_C_INCLUDES += \
	$(TI_STA_INCLUDES) \
	hardware/libhardware_legacy/include 
LOCAL_STATIC_LIBRARIES += libWifiApi
LOCAL_CPPFLAGS +=-DENABLE_TI_WIFI
LOCAL_CFLAGS +=-DENABLE_TI_WIFI
LOCAL_SRC_FILES +=ConnectivityLocalAndroid.cpp
LOCAL_LDLIBS += \
	-lwpa_client \
	-lhardware_legacy \
	-lcutils
LOCAL_SHARED_LIBRARIES += \
	libwpa_client \
	libhardware_legacy \
	libcutils
else
# Generic Linux device with wireless extension compliant wireless
# driver
LOCAL_SRC_FILES += ConnectivityLocalLinux.cpp
endif

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := libhagglekernel_jni

# LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_BIN_UNSTRIPPED)

# Charm Crypto Bridge Python Blob - Embedded inside the binary.
CCB_PYTHON_FILE=../../../ccb/python/ccb.py
LOCAL_LDFLAGS += -Wl,--format=binary -Wl,$(CCB_PYTHON_FILE) -Wl,--format=elf

include $(BUILD_SHARED_LIBRARY)

endif
