package org.haggle;

public interface EventHandler {
	// Shutdown event: haggle is shutting down.
	static final int EVENT_HAGGLE_SHUTDOWN = 0;
	// Neighbor update event.
        static final int EVENT_NEIGHBOR_UPDATE = 1;
	// New data object event.
	static final int EVENT_NEW_DATAOBJECT = 2;
        // Interest list of application
	static final int EVENT_INTEREST_LIST_UPDATE = 3;
        
	// These methods are the standard callback functions
        // and should be overridden in the class that implements
        // this interface
	void onNeighborUpdate(Node[] neighbors);
	void onNewDataObject(DataObject dObj);
	void onInterestListUpdate(Attribute[] interests);
	void onShutdown(int reason);
	void onEventLoopStart();
	void onEventLoopStop();
}
