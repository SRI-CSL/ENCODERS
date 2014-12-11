//
// nodeman.h
//
// Node Manager
//

#ifndef _nodeman_
#define _nodeman_

#include "model.h"

class NodeMan
{
public:
    ~NodeMan();

    static NodeMan *instance();
    void init(int numNode);

    Node *allocNode(int num);
    bool isInitPos(node_id_t id);

protected:
    NodeMan();
    static NodeMan *instance_;

    Node *nodes_;
    int numNodes_;
    int allocated_;

    bool initialized_;
};

#endif
