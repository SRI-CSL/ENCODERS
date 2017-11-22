# This target is the final Haggle executable
bin_PROGRAMS:=haggle
haggle_SOURCES:=main.cpp
haggle_DEPENDENCIES:=$(top_srcdir)/config.h $(top_srcdir)/src/hagglekernel/git_ref.h libhagglekernel.a $(top_builddir)/src/utils/libhaggleutils.a $(top_builddir)/src/libcpphaggle/libcpphaggle.a
haggle_LDFLAGS :=libhagglekernel.a $(top_builddir)/src/utils/libhaggleutils.a $(top_builddir)/src/libcpphaggle/libcpphaggle.a -lm -lcrypto -lsqlite3 $(XML_LIBS)
haggle_CXXFLAGS :=$(CXXFLAGS)
haggle_CPPFLAGS :=-Wall -fstack-protector-all $(XML_CPPFLAGS) -I$(top_srcdir)/src/utils -I$(top_srcdir)/src/libcpphaggle/include -I$(top_srcdir)/src/libhaggle/include
haggle_LDADD:= -lrt

haggle_CPPFLAGS += $(filter-out -Wstrict-prototypes -Werror=format-security,$(PYCFLAGS))
haggle_LDFLAGS += $(PYLDFLAGS)

BUILT_SOURCES = rm_git_ref.h git_ref.h

# This target is an intermediate library, that can be reused by the testsuite
noinst_LIBRARIES:=libhagglekernel.a
libhagglekernel_a_SOURCES = \
	Filter.cpp \
	Event.cpp \
	Attribute.cpp \
	Bloomfilter.cpp \
	DataObject.cpp \
	Node.cpp \
	Address.cpp \
	Interface.cpp \
	Certificate.cpp \
	RepositoryEntry.cpp \
	NodeStore.cpp \
	InterfaceStore.cpp \
	DataStore.cpp \
	SQLDataStore.cpp \
	MemoryDataStore.cpp \
	Metadata.cpp \
	XMLMetadata.cpp \
	MetadataParser.cpp \
	HaggleKernel.cpp \
	ConnectivityInterfacePolicy.cpp \
	Queue.cpp \
	Policy.cpp \
	Manager.cpp \
	NodeManager.cpp \
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
	Scratchpad.cpp \
	ScratchpadManager.cpp \
	EvictStrategyFactory.cpp \
	EvictStrategyManager.cpp \
	EvictStrategy.cpp \
	EvictStrategyLRFU.cpp \
	EvictStrategyLRU_K.cpp \
	ProtocolManager.cpp \
	ForwardingClassifier.cpp \
	ForwardingClassifierAllMatch.cpp \
	ForwardingClassifierBasic.cpp \
	ForwardingClassifierNodeDescription.cpp \
	ForwardingClassifierAttribute.cpp \
	ForwardingClassifierPriority.cpp \
	ForwardingClassifierSizeRange.cpp \
	ForwardingClassifierFactory.cpp \
	ForwardingManager.cpp \
	CharmCryptoBridge.cpp \
	SecurityManager.cpp \
	Forwarder.cpp \
	ForwarderFactory.cpp \
	ForwarderAggregate.cpp \
	ForwarderFlood.cpp \
	ForwarderNone.cpp \
	ForwarderAsynchronous.cpp \
	ForwarderAsynchronousInterface.cpp \
	ForwarderProphet.cpp \
	ForwarderAlphaDirect.cpp \
	Connectivity.cpp \
	ConnectivityLocal.cpp \
	ConnectivityManager.cpp \
	ApplicationManager.cpp \
	InterestManager.cpp \
	SocketWrapper.cpp \
	SocketWrapperTCP.cpp \
	SocketWrapperUDP.cpp \
	Protocol.cpp \
	ProtocolSocket.cpp \
	ProtocolUDPUnicastProxy.cpp \
	ProtocolUDPBroadcastProxy.cpp \
	ProtocolUDPUnicast.cpp \
	ProtocolUDPBroadcast.cpp \
	ProtocolUDP.cpp \
	ProtocolUDPGeneric.cpp \
	ProtocolClassifier.cpp \
	ProtocolClassifierAttribute.cpp \
	ProtocolClassifierBasic.cpp \
	ProtocolClassifierNodeDescription.cpp \
	ProtocolClassifierSizeRange.cpp \
	ProtocolClassifierAllMatch.cpp \
	ProtocolClassifierPriority.cpp \
	ProtocolClassifierFactory.cpp \
	ProtocolConfiguration.cpp \
	ProtocolConfigurationFactory.cpp \
	ProtocolConfigurationTCP.cpp \
	ProtocolConfigurationUDPGeneric.cpp \
	ProtocolFactory.cpp \
	ProtocolTCP.cpp \
	ProtocolLOCAL.cpp \
	ResourceManager.cpp \
	ResourceMonitor.cpp \
	Trace.cpp \
	Utility.cpp \
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
	fragmentation/managermodule/encoder/FragmentationEncoderAsynchronousManagerModule.cpp \
	fragmentation/manager/FragmentationManager.cpp \
	fragmentation/fragment/FragmentationFileUtility.cpp \
	fragmentation/service/FragmentationEncoderService.cpp \
	fragmentation/service/FragmentationDecoderService.cpp \
	fragmentation/storage/FragmentationDecoderStorage.cpp \
	fragmentation/utility/FragmentationDataObjectUtility.cpp \
	fragmentation/protocol/FragmentationProtocolHelper.cpp \
	fragmentation/configuration/FragmentationConfiguration.cpp \
	dataobject/DataObjectTypeIdentifierUtility.cpp \
	time/TimeStampUtility.cpp \
	fragmentation/concurrent/encoder/FragmentationEncodingTask.cpp \
	fragmentation/concurrent/decoder/FragmentationDecodingTask.cpp \
	fragmentation/managermodule/decoder/FragmentationDecoderAsynchronousManagerModule.cpp \
	fragmentation/storage/FragmentationEncoderStorage.cpp \
	fragmentation/manager/FragmentationSendSuccessFailureHandler.cpp \
	fragmentation/forwarding/FragmentationForwardingManagerHelper.cpp \
	fragmentation/bloomfilter/FragmentationBloomFilterHelper.cpp \
	stringutils/CSVUtility.cpp \
	LossEstimateManager.cpp \
	LossRateSlidingWindowElement.cpp \
	LossRateSlidingWindowEstimator.cpp


libhagglekernel_a_DEPENDENCIES=$(top_srcdir)/config.h

EXTRA_DIST_OTHER_PLATFORMS = \
	ConnectivityLocalAndroid.h \
	ConnectivityLocalAndroid.cpp \
	ConnectivityLocalWindowsXP.h \
	ConnectivityLocalWindowsXP.cpp \
	ConnectivityLocalWindowsMobile.h \
	ConnectivityLocalWindowsMobile.cpp \
	ConnectivityBluetoothWindowsXP.h \
	ConnectivityBluetoothWindowsXP.cpp \
	ConnectivityBluetoothWindowsMobile.h \
	ConnectivityBluetoothWindowsMobile.cpp \
	ConnectivityBluetoothWindowsMobileWIDCOMM.h \
	ConnectivityBluetoothWindowsMobileWIDCOMM.cpp \
	ProtocolRFCOMMWIDCOMM.h \
	ProtocolRFCOMMWIDCOMM.cpp \
	ResourceMonitorAndroid.h \
	ResourceMonitorAndroid.cpp \
	ResourceMonitorWindowsMobile.h \
	ResourceMonitorWindowsMobile.cpp \
	ResourceMonitorWindowsXP.h \
	jni.cpp \
	Android.mk

EXTRA_DIST= Attribute.h \
	Bloomfilter.h \
	ApplicationManager.h \
	ApplicationManager.h \
	ConnectivityManager.h \
	Connectivity.h \
	ConnectivityLocal.h \
	ConnectivityLocalMacOSX.h \
	ConnectivityLocalLinux.h \
	ConnectivityBluetooth.h \
	ConnectivityBluetoothLinux.h \
	ConnectivityBluetoothMacOSX.h \
	ConnectivityEthernet.h \
	ConnectivityMedia.h \
	Forwarder.h \
	ForwarderFactory.h \
	ForwarderProphet.h \
	ForwarderAlphaDirect.h \
	ForwarderAggregate.h \
	ForwarderFlood.h \
	ForwarderNone.h \
	ForwarderAsynchronous.h \
	ForwarderAsynchronousInterface.h \
	DataManager.h \
	SendPriorityManager.h \
	MemoryCache.h \
	ReplicationManagerFactory.h \
	ReplicationManager.h \
	ReplicationManagerModule.h \
	ReplicationManagerAsynchronous.h \
	ReplicationManagerUtility.h \
	ReplicationConnectState.h \
	ReplicationNodeDurationAverager.h \
	ReplicationSendRateAverager.h \
	ReplicationConcurrentSend.h \
	ReplicationOptimizer.h \
	ReplicationGlobalOptimizerFactory.h \
	ReplicationGlobalOptimizer.h \
	ReplicationKnapsackOptimizerFactory.h \
	ReplicationKnapsackOptimizer.h \
	ReplicationUtilityFunctionFactory.h \
	ReplicationUtilityFunction.h \
	CacheStrategy.h \
	CacheStrategyAsynchronous.h \
	CacheStrategyUtility.h \
	CacheUtilityFunction.h \
	CacheUtilityFunctionFactory.h \
	CacheKnapsackOptimizer.h \
	CacheKnapsackOptimizerFactory.h \
	CacheGlobalOptimizer.h \
	#CacheReplGlobalOptimizer.h \
	CacheGlobalOptimizerFactory.h \
	#CacheReplGlobalOptimizerFactory.h \
	CacheStrategyFactory.h \
	CacheStrategyReplacementPurger.h \
	CacheReplacement.h \
	CacheReplacementFactory.h \
	CacheReplacementTotalOrder.h \
	CacheReplacementPriority.h \
	CachePurger.h \
	CachePurgerFactory.h \
	CachePurgerAbsTTL.h \
	CachePurgerRelTTL.h \
	CachePurgerParallel.h \
	DataObject.h \
	DataStore.h \
	SQLDataStore.h \
	MemoryDataStore.h \
	Certificate.h \
	NodeStore.h \
	InterfaceStore.h \
	Scratchpad.h \
	ScratchpadManager.h \
	EvictStrategyFactory.h \
	EvictStrategy.h \
	EvictStrategyManager.h \
	EvictStrategyLRFU.h \
	EvictStrategyLRU_K.h \
	DebugManager.h \
	Debug.h \
	BenchmarkManager.h \
	Event.h \
	EventQueue.h \
	Filter.h \
	HaggleKernel.h \
	ConnectivityInterfacePolicy.h \
	Queue.h \
	Address.h \
	Interface.h \
	Manager.h \
	ManagerModule.h \
	Metadata.h \
	MetadataParser.h \
	Node.h \
	NodeManager.h \
	SecurityManager.h \
	SocketWrapper.h \
	SocketWrapperTCP.h \
	SocketWrapperUDP.h \
	Protocol.h \
	ProtocolSocket.h \
	ProtocolUDPBroadcastProxy.h \
	ProtocolUDPBroadcast.h \
	ProtocolUDPUnicastProxy.h \
	ProtocolClassifier.h \
	ProtocolClassifierAttribute.h \
	ProtocolClassifierBasic.h \
	ProtocolClassifierNodeDescription.h \
	ProtocolClassifierSizeRange.h \
	ProtocolClassifierSizeAllMatch.h \
	ProtocolClassifierSizePriority.h \
	ProtocolClassifierFactory.h \
	ProtocolConfiguration.h \
	ProtocolConfigurationFactory.h \
	ProtocolConfigurationTCP.h \
	ProtocolConfigurationUDPGeneric.h \
	ProtocolFactory.h \
	ProtocolLOCAL.h \
	ProtocolManager.h \
	ForwardingClassifier.h \
	ForwardingClassifierAllMatch.h \
	ForwardingClassifierBasic.h \
	ForwardingClassifierNodeDescription.h \
	ForwardingClassifierAttribute.h \
	ForwardingClassifierPriority.h \
	ForwardingClassifierSizeRange.h \
	ForwardingClassifierFactory.h \
	ForwardingManager.h \
	ProtocolRAW.h \
	ProtocolTCP.h \
	ProtocolUDP.h \
	ProtocolUDPGeneric.h \
	ProtocolRFCOMM.h \
	ProtocolRFCOMMMacOSX.h \
	ProtocolRFCOMMMacOSX.mm \
	ProtocolMedia.h \
	Policy.h \
	RepositoryEntry.h \
	XMLMetadata.h \
	ResourceManager.h \
	ResourceMonitor.h \
	ResourceMonitorLinux.h \
	ResourceMonitorMacOSX.h \
	Trace.h \
	Utility.h \
	htypes.h \
	CharmCryptoBridge.h \
	$(EXTRA_DIST_OTHER_PLATFORMS) \
	Doxyfile.in \
	hagglesim

libhagglekernel_a_CPPFLAGS:=$(haggle_CPPFLAGS)

if HAVE_EXCEPTION
haggle_CXXFLAGS +=-DHAVE_EXCEPTION=1
else
haggle_CXXFLAGS +=-DHAVE_EXCEPTION=0 -fno-exceptions -fno-non-call-exceptions
endif

# Configuration dependent source code
if DEBUG_LEAKS
libhagglekernel_a_SOURCES += Debug.cpp DebugManager.cpp 
endif

if DEBUG
libhagglekernel_a_SOURCES += Debug.cpp DebugManager.cpp 
endif

libhagglekernel_a_SOURCES += BenchmarkManager.cpp

# Linux specific source code
if OS_LINUX

haggle_LDFLAGS += -lpthread

libhagglekernel_a_SOURCES += ConnectivityLocalLinux.cpp ResourceMonitorLinux.cpp

if ENABLE_BLUETOOTH
libhagglekernel_a_SOURCES += ConnectivityBluetooth.cpp ConnectivityBluetoothLinux.cpp ProtocolRFCOMM.cpp
endif

if ENABLE_ETHERNET
libhagglekernel_a_SOURCES += ConnectivityEthernet.cpp
endif

if HAVE_DBUS
haggle_LDFLAGS += $(DBUS_LIBS)
haggle_CPPFLAGS += $(DBUS_INCLUDES)
endif


endif # OS_LINUX

# Objective C++ sources for Mac OS X APIs
OBJCXX_SRC = ProtocolRFCOMMMacOSX.mm
OBJCXX_SRC_CPP := $(OBJCXX_SRC:%.mm=%.cpp)

# Link the objective C++ source (.mm) to a .cpp file. Automake does
# not seem to recognize Objective C++ files.
$(OBJCXX_SRC_CPP): %.cpp: %.mm
	ln -s $< $@

# Mac OS X specific source code
if OS_MACOSX

libhagglekernel_a_SOURCES += ConnectivityLocalMacOSX.cpp ResourceMonitorMacOSX.cpp

if OS_IPHONEOS
haggle_LDFLAGS +=-framework CoreFoundation -L@OPENSSL_LIB_DIR@
else
haggle_LDFLAGS +=-framework CoreFoundation -framework IOKit -framework CoreServices
endif

if ENABLE_BLUETOOTH

libhagglekernel_a_SOURCES += ConnectivityBluetooth.cpp ConnectivityBluetoothMacOSX.cpp

noinst_LIBRARIES += libmacosxobjc.a
haggle_DEPENDENCIES += libmacosxobjc.a

libmacosxobjc_a_SOURCES := ProtocolRFCOMMMacOSX.cpp
libmacosxobjc_a_DEPENDENCIES=$(top_srcdir)/config.h

libmacosxobjc_a_CPPFLAGS :=$(haggle_CPPFLAGS) -x objective-c++ 
#libmacosxobjc_a_CXXFLAGS = 
haggle_LDADD += libmacosxobjc.a
haggle_LDFLAGS +=-framework IOBluetooth -framework Foundation
endif

if ENABLE_ETHERNET
libhagglekernel_a_SOURCES += ConnectivityEthernet.cpp
endif

if ENABLE_MEDIA
libhagglekernel_a_SOURCES += ConnectivityMediaMacOSX.cpp ProtocolMedia.cpp
endif

endif # OS_MACOSX

if ENABLE_ETHERNET
libhagglekernel_a_SOURCES += ConnectivityEthernet.cpp
endif

if OMNETPP
# OMNet++ support does not work in its current state
libhaggleopp_a_CPPFLAGS = -DOMNETPP 
noinst_LIBRARIES += libhaggleopp.a
libhaggleopp_a_SOURCES = HaggleKernel.cpp \
			Event.cpp \
			Node.cpp \
			DataObject.cpp \
			Interface.cpp \
			Attribute.cpp \
			DataManager.cpp \
			NodeManager.cpp \
			ProtocolManager.cpp \
			ConnectivityManager.cpp
endif

if BUNDLED_SQLITE
haggle_CPPFLAGS +=-I@SQLITE_INCLUDE_DIR@
haggle_CXXFLAGS +=-I@SQLITE_INCLUDE_DIR@
haggle_LDADD +=$(top_builddir)/$(SQLITE_SUBDIR)/libsqlite3.la
haggle_DEPENDENCIES +=$(top_builddir)/$(SQLITE_SUBDIR)/libsqlite3.la
haggle_LDFLAGS +=-static
else
haggle_LDFLAGS +=-lsqlite3 
endif

if ENABLE_BLUETOOTH
if OS_LINUX
haggle_LDFLAGS +=-lbluetooth
endif

if OS_MACOSX
haggle_LDFLAGS +=-framework IOBluetooth
endif
endif

if OS_MACOSX
haggle_LDFLAGS +=-framework CoreFoundation
endif

$(top_builddir)/bin/$(PROGRAMS): $(PROGRAMS)
	cp $(PROGRAMS) $@

all-local: $(top_builddir)/bin/$(PROGRAMS)

indent:
	indent -kr -i8 $(wildcard *.cpp) $(wildcard *.h)

clean-local:
	rm -f *~
	rm -f hagglesim/*~
	rm -f hagglesim/*_n.cc
	rm -f hagglesim/*_n.o
if OS_MACOSX
	for f in $(libmacosxobjc_a_SOURCES); do \
		rm -f $(f); \
	done
endif

# Clean up the hagglesim dir
dist-hook:
	rm -rf `find $(distdir)/hagglesim -name .svn`
	rm -f $(distdir)/hagglesim/*~
	rm -f $(distdir)/hagglesim/*_n.cc
	rm -f $(distdir)/hagglesim/*_n.o

rm_git_ref.h:
	rm -f git_ref.h

git_ref.h:
	(cd ../..; ./mkgitref)

# Charm Crypto Bridge Python Blob - Embedded inside the binary.
libhagglekernel_a_DEPENDENCIES += libhagglekernel_a-CharmCryptoBridgePythonBlob.o
libhagglekernel_a_LIBADD:=libhagglekernel_a-CharmCryptoBridgePythonBlob.o
OBJDUMP_LINUX=objdump
OBJCOPY_LINUX=objcopy
CCB_PYTHON_FILE=../../../ccb/python/ccb.py

libhagglekernel_a-CharmCryptoBridgePythonBlob.o: libhagglekernel_a-HaggleKernel.o
	$(eval ELF := $(shell $(OBJDUMP_LINUX) -f libhagglekernel_a-HaggleKernel.o | grep format | awk '{print $$NF}'))
	$(eval ARCH := $(shell $(OBJDUMP_LINUX) -f libhagglekernel_a-HaggleKernel.o | grep architecture | awk '{print $$2}' | sed 's/.$$//'))
	$(OBJCOPY_LINUX) -I binary -O $(ELF) -B $(ARCH) $(CCB_PYTHON_FILE) libhagglekernel_a-CharmCryptoBridgePythonBlob.o
