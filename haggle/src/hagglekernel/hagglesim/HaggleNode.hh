#ifndef _HAGGLENODE_HH
#define _HAGGLENODE_HH

#include <omnetpp.h>

#include "HaggleKernel.hh"
#include "ProtocolOMNETPP.hh"

using namespace std;

class HaggleNode : public cSimpleModule {
private:
	HaggleKernel *haggle;
	unsigned int nodeID;
	ProtocolOMNETPP *prot;
protected:
	virtual void initialize();
	virtual void handleMessage(cMessage *msg);    
	virtual void finish();

public:
	void sendMessage(cMessage *msg);
	HaggleNode();
	~HaggleNode() {}
};

#endif
