#include <omnetpp.h>

#include "DataManager.hh"
#include "NodeManager.hh"
#include "ProtocolManager.hh"
#include "ConnectivityManager.hh"
#include "utils.h"
#include "ProtocolOMNETPP.hh"

#include "HaggleNode.hh"

// register module class with OMNeT++
Define_Module(HaggleNode);

HaggleNode::HaggleNode() {
	haggle = new HaggleKernel;
	
	if (!haggle) {
		cerr << "Haggle startup error!" << endl;
	 	return;		
	}
	
	DataManager *dm = new DataManager(haggle);
	haggle->registerManager(dm);
	
	NodeManager *nm = new NodeManager(haggle);
	haggle->registerManager(nm);
	
	ProtocolManager *pm = new ProtocolManager(haggle);
	prot = new ProtocolOMNETPP(this);

	pm->addProtocol(prot);
	haggle->registerManager(pm);

	ConnectivityManager *cm = new ConnectivityManager(haggle);
	haggle->registerManager(cm);
}

void HaggleNode::initialize()
{
	ev << "Haggle Node " << nodeID << "\n";
	cMessage *msg = new cMessage("tictocMsg");
	sendMessage(msg);
}

void HaggleNode::sendMessage(cMessage *msg)
{
	send(msg, "out");
}

void HaggleNode::handleMessage(cMessage *msg)
{
	// 
	//prot->receive(
	// Immediately send back.
	sendMessage(msg);
}
void HaggleNode::finish()
{
	delete haggle;
}
