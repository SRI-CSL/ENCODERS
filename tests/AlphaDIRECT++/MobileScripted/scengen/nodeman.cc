//
// nodeman.cc
// 
// Node Manager
//

#include "defs.h"
#include "nodeman.h"

NodeMan *NodeMan::instance_ = NULL;

// constructor
NodeMan::NodeMan()
{
    nodes_ = NULL;
    numNodes_ = 0;
    allocated_ = 0;

    initialized_ = false;
}

// destructor
NodeMan::~NodeMan()
{
    if (nodes_) {
        delete nodes_;
    }
}

// access to the instance
NodeMan *NodeMan::instance()
{
    if (instance_ == NULL) {
        instance_ = new NodeMan();
        assert(instance_);
    }
    return instance_;
}

// initialize the nodes
void NodeMan::init(int numNodes)
{
    assert(numNodes > 0);
    numNodes_ = numNodes;

    nodes_ = new Node[numNodes];
    assert(nodes_);

    initialized_ = true;
}

// allocate some nodes
Node *NodeMan::allocNode(int num)
{
    assert(initialized_ && num > 0 && num + allocated_ <= numNodes_);

    Node *handle = &(nodes_[allocated_]);
    assert(handle);
    allocated_ += num;

    return handle;
}

// query for the status of the node, whether it contains initial position only
bool NodeMan::isInitPos(node_id_t id)
{
    assert(id >= 0 && id < (unsigned)numNodes_);

    bool result = (nodes_[id].numMoves_ <= 1);
    return result;
}

