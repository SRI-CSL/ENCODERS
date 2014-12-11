#include "ProtocolOMNETPP.hh"

ProtocolOMNETPP::ProtocolOMNETPP(HaggleNode *_hnode) : Protocol(PROT_TYPE_OMNETPP), hnode(_hnode)
{
}

ProtocolOMNETPP::~ProtocolOMNETPP()
{
}


int ProtocolOMNETPP::send(char *buf, int len, struct in_addr _dest)
{
	return 0;
}


int ProtocolOMNETPP::receive(char *buf, int buflen)
{
	return 0;
}
