#ifndef _PROTOCOLOMNETPP_HH
#define _PROTOCOLOMNETPP_HH

#include <netinet/in.h>

#include "Protocol.hh"

class HaggleNode;

class ProtocolOMNETPP : public Protocol {
public:
	ProtocolOMNETPP(HaggleNode *_hnode);
	~ProtocolOMNETPP();
	void setDest(struct in_addr _dest) { dest = _dest; }
	int send(char *buf, int buflen, struct in_addr _dest);
	int send(char *buf, int buflen) { return send(buf, buflen, dest); }
	int receive(char *buf, int buflen);
private:	
	HaggleNode *hnode;
	struct in_addr dest;
	int flags;
};

#endif /* PROTOCOLOMNETPP_HH */
