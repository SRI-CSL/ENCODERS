//
// pursue.cc
//
// Pursue Motion Model
//

#include "defs.h"

#include "pursue.h"

// constructor
Pursue::Pursue(long seed) : Model(seed)
{
    type_ = MODEL_PURSUE;

    thief_ = NULL;
    V_min_ = 0;
    V_max_ = 0;
    T_ = 0;

    police_ = NULL;
    V_pmin_ = 0;
    V_pmax_ = 0;
}

// destructor
Pursue::~Pursue()
{
}

// Member functions

// initialized
void Pursue::init(model_time_t startTime, \
                  model_time_t stopTime, \
                  node_id_t startID, \
                  int num_nodes, \
                  Area *area, \
                  bool cp)
{
    // make sure at least 2 nodes
    assert(num_nodes > 1 && paramList_);

    Model::init(startTime, stopTime, startID, num_nodes, area, cp);

    T_ = getf("T");
    V_min_ = getf("V_min");
    V_max_ = getf("V_max");
    V_pmin_ = getf("V_pmin");
    V_pmax_ = getf("V_pmax");

    // some validation of the params should be done here
    assert(T_ > 0 && V_max_ >= V_min_ && V_min_ >= 0 && 
           V_pmax_ >= V_pmin_ && V_pmin_ >= 0);

    thief_ = &(nodes_[0]);
    police_ = &(nodes_[1]);

    //assert(thief_ && police_);

    initialized_ = true;
}

// In a Pursue Motion Model, one node (the thief_) is moving in a way similar
// to that in a Random Waypoint Model with a zero pause time, and speed
// uniformly distributed in (V_min_, V_max_).
//
// The movements of the other nodes (police_) are devided into discrete time 
// intervals (T_), and at the begining of each time interval, they choose a
// destination point where the thief_ would be at the end of this interval, 
// and a speed uniformly distributed in (V_pmin_, V_pmax_)
void Pursue::makeMove(Node *node)
{
    assert(node);

    if (node->numMoves_ == 0) {
        // set initial position only
        area_->randomPos(node->pos_);
        node->dest_ = node->pos_;

        node->startTime_ = startTime_;
        node->arrivalTime_ = startTime_;

        // assume zero pause time
        node->nextStartTime_ = startTime_;

        node->speed_ = 0.0;
    } else {
        // update pos_ and startTime_ of the node first
        node->pos_ = node->dest_;
        node->startTime_ = node->nextStartTime_;

        if (node == thief_) {
            // it is the thief_, update thief_ similar to random waypoint
            // model

            // get a random virtual destination
            area_->randomPos(node->dest_);

            // choose a random speed
            node->speed_ = getRandomDouble(DIST_UNIFORM, V_min_, V_max_);

            // calculate the time it takes for this move
            model_time_t t = (node->dest_ - node->pos_).length()/node->speed_;
            node->arrivalTime_ = node->startTime_ + t;

            /*
            cerr << "Setting thief_: dest: " 
                 << node->dest_.x_ << ", " 
                 << node->dest_.y_ 
                 << " at speed: " << node->speed_
                 << "\n";
                 */

        } else {
            // it is a police node. just try pursue the thief

            // choose a random speed
            node->speed_ = getRandomDouble(DIST_UNIFORM, V_pmin_, V_pmax_); 

            // calculate the position the thief_ would be
            // at the end of this time interval
            Vector dest = thief_->pos_ + (thief_->dest_ - thief_->pos_) * 
                ((node->startTime_ + T_ - thief_->startTime_) / 
                (thief_->arrivalTime_ - thief_->startTime_));

            // the time it takes for the police to move the that dest
            model_time_t t = (dest - node->pos_).length() / node->speed_;

            // calculate the real dest for the police
            node->dest_ = node->pos_ + (dest - node->pos_) * (T_/t);
            node->arrivalTime_ = node->startTime_ + T_;
        }

        // in either case, there's zero pause
        node->nextStartTime_ = node->arrivalTime_;
    }
}

