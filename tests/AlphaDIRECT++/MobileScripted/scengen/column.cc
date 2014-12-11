//
// column.cc
//
// Column Motion Model
//

#include "defs.h"

#include "column.h"

// constructor
Column::Column(long seed) : Model(seed)
{
    type_ = MODEL_COLUMN;

    V_min_ = 0;
    V_max_ = 0;
    T_ = 0;
}

// destructor
Column::~Column()
{
}

// Member functions
void Column::init(model_time_t startTime,
                  model_time_t stopTime,
                  node_id_t startID,
                  int num_nodes,
                  Area *area, 
                  bool cp)
{
    //assert(num_nodes > 0 && paramList_);
    assert(num_nodes == 1 && paramList_);

    Model::init(startTime, stopTime, startID, num_nodes, area, cp);

    T_ = getf("T");
    V_min_ = getf("V_min");
    V_max_ = getf("V_max");

    assert(T_ > 0 && V_min_ >= 0 && V_max_ > 0 && V_max_ >= V_min_);

    if (paramList_->get("A")) {
        // angle of the initial velocity
        A_ = getf("A");
    } else {
        // default is the positive X direction
        A_ = 0;
    }

    initialized_ = true;
}

// In the Column Motion Model, the node moves along a straight line.
// The initial position of the node is (0, 0), unless otherwise specified 
// by "X" and "Y" in the paramList_.
//
// The initial direction movement is specified by "A" in the paramList_.
// It is default to 0pi, which is the positive X direction.
//
// The motion of the nodes is devided into discrete time intervals. At the
// begining of each time interval, each node randomly choose a speed between
// V_min_ and V_max_. If a movement would cause a node to move out of the 
// area, the direction of the speed is reversed.
void Column::makeMove(Node *node)
{
    assert(node);

    if (node->numMoves_ == 0) {
        // first move
        // set initial positions

        if (paramList_->get("X") && paramList_->get("Y")) {
            // if the pos is specified by the user
            double x = getf("X");
            double y = getf("Y");
            Vector tmp(x, y);
            assert(area_->inside(tmp));

            node->pos_.x_ = x;
            node->pos_.y_ = y;
        } else {
            // else just randomly choose one
            area_->randomPos(node->pos_);
        }

        node->dest_ = node->pos_;

        node->startTime_ = startTime_;
        node->arrivalTime_ = startTime_;
        node->nextStartTime_ = startTime_;
        node->speed_ = 0;

    } else {
        // choose a speed
        node->speed_ = getRandomDouble(DIST_UNIFORM, V_min_, V_max_);

        Vector dest;

        // the new coordinate would be
        dest.x_ = node->dest_.x_ + node->speed_ * cos(A_*M_PI) * T_;
        dest.y_ = node->dest_.y_ + node->speed_ * sin(A_*M_PI) * T_;

        if (! area_->inside(dest) ) {
            // if it is out of the area, flip the direction
            A_++;
        }

        // re-calculate the coordinate
        dest.x_ = node->dest_.x_ + node->speed_ * cos(A_*M_PI) * T_;
        dest.y_ = node->dest_.y_ + node->speed_ * sin(A_*M_PI) * T_;

        // the new coordinate has to be inside the area, otherwise the 
        // parameters must have been set wrongly.
        assert(area_->inside(dest));

        // update the node
        node->pos_ = node->dest_;
        node->dest_ = dest;
        node->startTime_ = node->nextStartTime_;
        node->arrivalTime_ = node->startTime_ + T_;
        node->nextStartTime_ = node->arrivalTime_;
    }
}
