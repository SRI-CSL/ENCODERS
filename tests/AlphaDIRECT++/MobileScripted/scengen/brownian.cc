// 
// brownian.cc
//
// Brownian Motion Model
//

#include "defs.h"

#include "modelspec.h"
#include "brownian.h"
#include "nodeman.h"

#define MAX_T 1000
#define MAX_V 1000

// constructor
Brownian::Brownian(long seed) : Model(seed)
{
    type_ = MODEL_BROWNIAN;

    T_ = 0;
    V_ = 0;
    A_min_ = 0;
    A_max_ = 0;
}

// destructor
Brownian::~Brownian()
{
}

// Member functions

// initialization
void Brownian::init(model_time_t startTime, \
                    model_time_t stopTime, \
                    node_id_t startID, \
                    int num_nodes, \
                    Area *area, \
                    bool cp)
{
    Model::init(startTime, stopTime, startID, num_nodes, area, cp);
    assert(paramList_ != NULL);

    T_ = getf("T");
    V_ = getf("V");
    A_min_ = getf("A_min") * M_PI;
    A_max_ = getf("A_max") * M_PI;

    assert(T_ > 0 && T_ <= MAX_T && V_ > 0 && V_ <= MAX_V);

    initialized_ = true;
}

// make the move
// this implements the brownian motion
void Brownian::makeMove(Node *node)
{
    assert(node);

    //if (node->dest_.x_ <= 0 && node->dest_.y_ <= 0) {
    if (node->numMoves_ == 0) {
        // just initialized, this is the first move to be made
        // set inital position only

        area_->randomPos(node->pos_);
        node->dest_ = node->pos_;
        node->speed_ = 0.0;

    } else {
        // not the first move?

        // the speed and the angle
        double speed, alpha;
        // next destination
        Vector dest;
        // time spent on moving
        model_time_t move_time;

        bool done = false;
        while (!done) {
            // choose a random speed
            speed = getRandomDouble(DIST_UNIFORM, 0, V_);
            assert(speed > 0 && speed <= MAX_V);

            // choose a random angle
            alpha = getRandomDouble(DIST_UNIFORM, A_min_, A_max_);

            // the increment in x and y direction
            Vector incr(cos(alpha), sin(alpha));
            incr *= (speed * T_);

            dest = node->dest_ + incr;

            move_time = 0;

            if (area_->inside(dest)) {
                move_time = T_;
                done = true;
            } else {
                area_->findNearestPoint(dest);
                if (! (dest == node->dest_)) {
                    move_time = (dest - node->dest_).length() / speed;
                    done = true;
                }
            }
        }

        // update the node
        node->pos_ = node->dest_;
        node->dest_ = dest;
        node->startTime_ = node->nextStartTime_;
        node->arrivalTime_ = node->startTime_ + move_time;
        node->nextStartTime_ = node->startTime_ + T_;
        node->speed_ = speed;
    }
}

