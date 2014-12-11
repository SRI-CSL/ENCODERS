//
// waypoint.cc
//
// Random Waypoint Model

#include "defs.h"

#include "modelspec.h"
#include "waypoint.h"


// Constructor
Waypoint::Waypoint(long seed) : Model(seed)
{
    type_ = MODEL_WAYPOINT;

    T_max_ = 0;
    T_min_ = 0;
    T_dist_ = DIST_CONST;

    V_max_ = 10;
    V_min_ = 10;
    V_dist_ = DIST_CONST;

    randomPos_ = true;
}

// Destructor
Waypoint::~Waypoint()
{
}

// Member functions

// initialize the model. prepare for the movement generation
void Waypoint::init(model_time_t startTime, \
                    model_time_t stopTime, \
                    node_id_t startID, \
                    int num_nodes, \
                    Area *area, \
                    bool cp)
{
    Model::init(startTime, stopTime, startID, num_nodes, area, cp);

    assert(paramList_ != NULL);

    // init parameters
    T_max_ = getf("T_max");
    T_min_ = getf("T_min");
    getDistType(get("T_dist"), T_dist_);

    V_max_ = getf("V_max");
    V_min_ = getf("V_min");
    getDistType(get("V_dist"), V_dist_);

    initialized_ = true;
}

// assumed that posistions are randomly chosen
//
// for the Waypoint model, a node behaves like this: 
// step 1: pause for a certain time and then 
// step 2: (randomly) choose a destination and move to it with a 
//         (randomly) chosen speed. 
// after it reaches the destination, it repeats the above 2 steps
//
// Note: the "setdest" program in ns-src/indep-utils/cmu-scen-gen/setdest
// has a slightly different implementation, and is a subset of this one: 
// 1. it doesn't allow random pause time
// 2. it doesn't allow distribution types other than uniform distribution
void Waypoint::makeMove(Node *node)
{
    assert (node != NULL);

    if (node->numMoves_ == 0) {   // first move should be made
        // choose initial position only
        area_->randomPos(node->pos_);
        node->dest_ = node->pos_;

        node->startTime_ = startTime_;

        // this is the initial pause time
        node->arrivalTime_ = startTime_;
        node->nextStartTime_ = startTime_ + \
                               getRandomDouble(T_dist_, T_min_, T_max_);

        node->speed_ = 0.0;

    } else {                                // following moves
        // update position
        node->pos_ = node->dest_;     
        // choose a random destination
        area_->randomPos(node->dest_);

        // distance of the move
        double len = (node->dest_ - node->pos_).length();
        // get a random speed
        node->speed_ = getRandomDouble(V_dist_,V_min_, V_max_);

        // time takes to move
        model_time_t move_time = len / node->speed_;
        // pause time after arrival
        model_time_t pause_time = getRandomDouble(T_dist_, T_min_, T_max_);

        // update the times
        node->startTime_ = node->nextStartTime_;
        node->arrivalTime_ = node->startTime_ + move_time;
        node->nextStartTime_ = node->arrivalTime_ + pause_time;

    } // if-else
}
