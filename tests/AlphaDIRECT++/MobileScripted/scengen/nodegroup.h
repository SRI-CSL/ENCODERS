//
// nodegroup.h
// 
// Generic frame of a group model
//

#ifndef _nodegroup_
#define _nodegroup_

#include "model.h"

class NodeGroup
{
public:
    NodeGroup(long seed = 0);
    ~NodeGroup();

    // set the parameter that matches the name with the values in p1/p2
    void setParam(char *name, void *p1, void *p2 = NULL);

    // returns the total number of nodes in this group
    int init(node_id_t startID, \
             model_time_t globalStart, \
             model_time_t globalStop, \
             Area *globalArea);

    model_time_t nextMove(Move *move);

    // just for debugging
    void dump();

protected:
    // private functions
    void updateMoveList();
    void findMinTime(model_time_t &min_time, node_id_t &min_node);
    model_time_t findBreakPoint(Node *node, model_time_t min_time);
    // calculate the postion of node at time
    Vector calPos(Node *node, model_time_t time);

    // member variables
    bool initialized_;

    Node *cp_;              // the center point 
    Move cpMove_;           // the move that cp_ is taking

    int numNodes_;          // total number of nodes
    node_id_t startID_;     // the starting id
    Area *memberArea_;      // area where member nodes move 
                            // respective to the cp_

    Area *cpArea_;          // area where cp_ moves within
    Vector offset_;         // offset of the area of cp_ respective to origin

    Model *cpModel_;        // model used for cp_
    Model *memberModel_;    // model used for member nodes

    // start and stop time of this group
    model_time_t startTime_;
    model_time_t stopTime_;

    // need a list to store all the moves in progress.
    // its size is numNodes_
    // the index of a move is move->id_ - startID_
    Move *moveList_;
    // stores the next move temporarily if it should be processed later
    Move nextMove_;

    // direct access to the nodes
    Node *nodeList_;

    // status of the nodes: 0 -- first move, 1 -- subsequent moves
    int *nodeStatus_;

    long seed_;
};

#endif
