//
// nodegroup.cc
//
// NodeGroup class

#include "defs.h"

#include "modelspec.h"
#include "nodegroup.h"

// constructor
NodeGroup::NodeGroup(long seed)
{
    initialized_ = false;

    cp_ = NULL;

    numNodes_ = 0;
    startID_ = 1;

    memberArea_ = NULL;
    cpArea_ = NULL;

    cpModel_ = NULL;
    memberModel_ = NULL;

    startTime_ = 0;
    stopTime_ = 0;

    moveList_ = NULL;
    nodeList_ = NULL;
    bzero(&nextMove_, sizeof(Move));
    nextMove_.time_ = -1;

    nodeStatus_ = NULL;
    seed_ = seed;
}

// destructor
NodeGroup::~NodeGroup()
{
    if (memberArea_) delete memberArea_;
    if (cpArea_) delete cpArea_;
    if (cpModel_) delete cpModel_;
    if (memberModel_) delete memberModel_;
    if (cp_) delete cp_;
}

// set the parameters
void NodeGroup::setParam(char *name, void *p1, void *p2)
{
    assert(name && p1);

    if (! strcmp(name, "num_nodes")) {
        assert (*(int *)p1 > 0);
        numNodes_ = *(int *)p1;
    } else if (! strcmp(name, "member_area")) {
        memberArea_ = new Area((char *)p1, seed_);
        assert(memberArea_);
    } else if (! strcmp(name, "center_area")) {
        cpArea_ = new Area((char *)p1, seed_);
        assert(cpArea_);
    } else if (! strcmp(name, "offset")) {
        assert (*(double *)p1 >= 0 && *(double *)p2 >= 0);
        offset_.x_ = *(double *)p1;
        offset_.y_ = *(double *)p2;
    } else if (! strcmp(name, "center_model")) {
        cpModel_ = ModelSpec::instance()->createModel((char *)p1, seed_);
        assert(cpModel_->type() != MODEL_UNKNOWN);
    } else if (strstr(name, "center_model.")) {
        // set parameter for the center model
        assert(cpModel_ != NULL && p2 != NULL);
        cpModel_->set((char *)p1, (char *)p2);
    } else if (! strcmp(name, "member_model")) {
        memberModel_ = ModelSpec::instance()->createModel((char *)p1, seed_);
        assert(memberModel_->type() != MODEL_UNKNOWN);
    } else if (strstr(name, "member_model.")) {
        // set parameter for the member model
        assert(memberModel_ != NULL && p2 != NULL);
        memberModel_->set((char *)p1, (char *)p2);
    } else if (! strcmp(name, "start_time")) {
        startTime_ = *(model_time_t *)p1;
        assert(startTime_ > 0);
    } else if (! strcmp(name, "stop_time")) {
        stopTime_ = *(model_time_t *)p1;
        assert(stopTime_ > 0);
    } else {
        // shouldn't happen
        fprintf(stderr, "NodeGroup: param %s not supported\n", name);
        assert(0);
    }
}

// initialize all models, returns the number of nodes
int NodeGroup::init(node_id_t startID, \
                    model_time_t globalStart, \
                    model_time_t globalStop, \
                    Area *globalArea)
{
    //cerr << "NodeGroup::init()" << endl;

    assert(startID >= 0);
    assert(numNodes_ > 0 && memberModel_);
    assert(globalStart >= 0 && globalStop > globalStart);
    assert(globalArea);

    if (startTime_ >= stopTime_) {  // the times should take global ones
        startTime_ = globalStart;
        stopTime_ = globalStop;
    } else {
        if (startTime_ < globalStart) {
            startTime_ = globalStart;
        }
        if (stopTime_ > globalStop) {
            stopTime_ = globalStop;
        }
    } // if (startTime_ == stopTime_)

    startID_ = startID;

    // init the center point
    if (cpModel_) {
        // cp has a model, then use that model
        if (!cpArea_) { // in case cpArea_ is not specified
            cpArea_ = globalArea;
        }

        cpModel_->init(startTime_, stopTime_, 0, 1, cpArea_, true);

        cp_ = cpModel_->getNode(0);
        assert(cp_);

        // get the initial position
        cpModel_->nextMove(&cpMove_);

        // update cp
        //cpModel_->nextMove(&cpMove_);
    } else {
        // cp does NOT have a model to use
        // just make it a fixed point
        // let's make it the origin (0,0)
        // this would be implicit if cp_ == NULL
    } // if-else

    // init the member nodes
    if (!memberArea_) {     // in case member area is not there
        memberArea_ = globalArea;
    }
    memberModel_->init(startTime_, stopTime_, \
                       startID_, numNodes_, \
                       memberArea_);

    // init the moveList_
    moveList_ = new Move[numNodes_];
    bzero(moveList_, sizeof(Move) * numNodes_);

    for (int i=0; i<numNodes_; i++) {
        moveList_[i].time_ = -1;
        moveList_[i].id_ = startID_ + i;
    }
    nodeList_ = memberModel_->getNodeList();
    assert(nodeList_);

    nodeStatus_ = new int[numNodes_];
    bzero(nodeStatus_, numNodes_*sizeof(int));

    initialized_ = true;

    return numNodes_;
}

// generate next movement
model_time_t NodeGroup::nextMove(Move *move)
{
    //cerr << "NodeGroup::nextMove()\n";
    assert(initialized_ && memberModel_);

    // if cp is not specified, the group is not moving
    // thus proceed with member node movement
    if (cp_ == NULL || cpModel_ == NULL) {
        model_time_t t = memberModel_->nextMove(move);
        move->dest_ += offset_;

        //cerr << "NodeGroup::nextMove() returns\n";
        return t;
    }

    //cerr << "cpModel_ applies\n";

    // else, cpModel_ should apply
    assert(cpModel_);

    // probe the member model to see whether we should update moveList_
    updateMoveList();

    //cerr << "MoveList updated\n";

    // find in the moveList_ the smallest time
    model_time_t min_time;
    // and the corresponding node index
    node_id_t min_node;

    findMinTime(min_time, min_node);

    // is simulation stopped?
    if (min_time < 0 || min_time >= stopTime_) {
        move->time_ = -1;
        //cerr << "NodeGroup::nextMove() return -1 " << min_time << endl;
        return -1;
    }

    //cerr << "Update cp_\n";

    // if no new movement during the current move of cp_, update cp_
    //if (min_time >= cp_->nextStartTime_) {
    while (min_time >= cp_->nextStartTime_) {
        //cerr << ".";
        cpModel_->nextMove(&cpMove_);
    }

    // find the smallest among cp_.arrivalTime_, cp_.nextStartTime_, 
    // node.arrivalTime_ and node.nextStartTime_, and which is also larger 
    // than min_time. call it bp, the breakpoint
    Node *node = &nodeList_[min_node];
    model_time_t bp = findBreakPoint(node, min_time);
    assert(bp >= min_time);

    // calculate the current speed and dest from min_time to bp
    // for the node and for the cp_
    Vector nodePos_start = calPos(node, min_time);
    Vector nodePos_stop = calPos(node, bp);
    Vector cpPos_start = calPos(cp_, min_time);
    Vector cpPos_stop = calPos(cp_, bp);

    Vector src = nodePos_start + cpPos_start;
    Vector dest = nodePos_stop + cpPos_stop;

    double speed;
    if (bp == min_time && min_time == startTime_) {
        // set initial position only
        speed = 0;
        // to prevent from entering inifite loop
        bp =+ INFINITESIMAL;
    } else {
        speed = (dest - src).length() / (bp - min_time);
    }

    // then form the move
    move->time_ = min_time;
    move->id_ = min_node + startID_;
    move->dest_ = dest + offset_;
    move->speed_ = speed;
    move->firstMove_ = (nodeStatus_[min_node] < 1);

    // update the start time for the move of the node
    assert(moveList_[min_node].time_ == min_time);

    //cerr << "min_time = " << min_time << " bp = " << bp << endl;
    moveList_[min_node].time_ = bp;

    //cerr << "NodeGroup::nextMove() return " << min_time << endl;

    if (move->speed_ == 0) {
        if (move->time_ > startTime_) {
            move->speed_ = INFINITESIMAL;
        }
    }

    // mark the node, the first move has been made
    nodeStatus_[move->id_-startID_] = 1;

    /*
    cerr << "NodeGroup::nextMove() returns "
         << "id: " << move->id_
         << ", time: " << move->time_
         << ", speed: " << move->speed_ << endl;
         */

    return min_time;
}


void NodeGroup::updateMoveList()
{
    //cerr << "NodeGroup::updateMoveList_()\n";

    assert(moveList_);

    Move *m = new Move();
    assert(m);

    bool done = false;
    while (!done) {
        // probe the member model
        model_time_t nextTime = memberModel_->nextMove(m, true);

        if (nextTime < 0 || nextTime > stopTime_) { // simulation stopped?
            done = true;
        } else {                // update moveList_
            int nodeIndex = m->id_ - startID_;
            //Node *n = &(nodeList_[nodeIndex]);
            Node &n = nodeList_[nodeIndex];
            if (moveList_[nodeIndex].time_ < 0 || \
                (moveList_[nodeIndex].time_ >= n.nextStartTime_ && \
                 nodeStatus_[nodeIndex] > 0) ) { 
                // not initialized, or
                // all move segments for this nodes are carried out already
                //      (and it is not the first move)
                // so, update the node by get the next move from model
                memberModel_->nextMove(&(moveList_[nodeIndex]));

                /*
                if (moveList_[nodeIndex].time_ != nextTime) {
                    cerr << moveList_[nodeIndex].time_ 
                        << " --- " << nextTime << endl;
                }
                */

                assert(moveList_[nodeIndex].time_ == nextTime);

            } else {
                done = true;
            } // if-else
        } // if-else
    } // while

    delete m;
}

void NodeGroup::findMinTime(model_time_t &min_time, node_id_t &min_node)
{
    // go through moveList_ to see whether there are nodes
    // to be re-calculated
    min_time = stopTime_ + 1;
    min_node = 0;

    for (int i=0; i<numNodes_; i++) {
        if (moveList_[i].time_ >= 0 && \
            moveList_[i].time_ < min_time) {
            min_time = moveList_[i].time_;
            min_node = i;
        }
    }
}

model_time_t NodeGroup::findBreakPoint(Node *node, model_time_t min_time)
{
    model_time_t t1, t2;

    if (min_time < cp_->arrivalTime_) {
        t1 = cp_->arrivalTime_;
    } else if (min_time < cp_->nextStartTime_) {
        t1 = cp_->nextStartTime_;
    } else {
        t1 = cp_->nextStartTime_+1;
        assert(0);
    }

    if (min_time == node->arrivalTime_ && \
        min_time == node->startTime_) {
        // this should the case when setting initial positions
        if (node->numMoves_ <= 1) {
            t2 = min_time;
        } else {
            //cerr << "Node has same startTime and arrivalTime\n";
            assert(0);
        }
    } else if (min_time < node->arrivalTime_) {
        t2 = node->arrivalTime_;
    } else if (min_time < node->nextStartTime_) {
        t2 = node->nextStartTime_;
    } else {
        t2 = cp_->nextStartTime_+1;
        assert(0);
    }

    return (t1 < t2) ? t1 : t2;
}

// calculate the position of node at the time
Vector NodeGroup::calPos(Node *node, model_time_t time)
{
    //assert(node && time >= node->startTime_ && time <= node->nextStartTime_);
    assert(node);

    //assert(time >= node->startTime_);
    if (time < node->startTime_) {
        fprintf(stderr, "node: %ld, time: %f, startTime: %f\n", \
                node->id_, \
                time, \
                node->startTime_);
        assert(0);
    }

    assert(time <= node->nextStartTime_);

    if (time >= node->arrivalTime_) {   // node stopped
        return node->dest_;
    } else {                            // node moving
        model_time_t t1 = time - node->startTime_;
        model_time_t t2 = node->arrivalTime_ - node->startTime_;

        if (t1 == 0 || t2 == 0) {
            return node->pos_;
        } else {
            return (node->pos_ + (node->dest_ - node->pos_) * (t1 / t2));
        }
    }
}


// just for debugging
void NodeGroup::dump()
{
    printf("num_nodes: %d\n", numNodes_);
    printf("start: %.12f\n", startTime_);
    printf("stop: %.12f\n", stopTime_);
    printf("offset: %.12f, %.12f\n", offset_.x_, offset_.y_);
    memberModel_->dump();
}

