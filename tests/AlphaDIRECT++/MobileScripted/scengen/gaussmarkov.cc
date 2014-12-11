//
// markov.cc
//
// The Gauss-Markov Model
//

#include "defs.h"
#include <math.h>
#include "gaussmarkov.h"

// Constructor
GaussMarkov::GaussMarkov(long seed) : Model(seed)
{
    type_ = MODEL_GAUSS_MARKOV;
    T_ = 0;
    SIGMA_ = 0;
    ALPHA_ = 0;
}

// Destructor
GaussMarkov::~GaussMarkov()
{
}

void GaussMarkov::init(model_time_t startTime,
                  model_time_t stopTime,
                  node_id_t startID,
                  int num_nodes,
                  Area *area,
                  bool cp)
{
    Model::init(startTime, stopTime, startID, num_nodes, area, cp);
    assert(paramList_!=NULL);

    T_ = getf("T");
    SIGMA_ = getf("SIGMA");
    ALPHA_ = getf("ALPHA");

    assert(T_ > 0 && SIGMA_ > 0 && ALPHA_ > 0 && ALPHA_ < 1);

    A_ = sqrt(1-ALPHA_*ALPHA_);
    initialized_ = true;
}

// The Gauss-Markov model updates the velocity of a node every T_ seconds.
// At each time interval, the node velocity v_{n} is given by:
//     v_{n} = ALPHA_ * v_{n-1} + (1-ALPHA) * u + \sqrt{1-ALPHA^2} * R
// where 0 < ALPHA < 1, u is the asymptotic mean of v_{n} when n->infinity
// and R is a uncorrelated gaussian process with 0 mean and standard 
// deviation SIGMA_.
// 
// This equation describes a 1-D Gauss-Markov motion. And it can be extended
// to 2-D motion by applying the formula to both X and Y direction components
// of the node velocity.
//
// In practice, since nodes are moving in a closed area, u is set to 0,
// v_{0} is set to 0, and the initial position of the node is randomly chosen
// in the simulation area.
void GaussMarkov::makeMove(Node *node)
{
    assert(node!=NULL);

    if (node->numMoves_ == 0) {         // the first move
        area_->randomPos(node->pos_);
        node->dest_ = node->pos_;

        node->startTime_ = startTime_;
        node->arrivalTime_ = startTime_;
        node->nextStartTime_ = startTime_;
        node->speed_ = 0.0;
    } else {                            // following moves
        // calculate the velocity of the last movement in both
        // x and y directions
        Vector v_last;
        model_time_t t_last = node->arrivalTime_ - node->startTime_;

        if (t_last == 0) {
            v_last *= 0;
        } else {
            v_last = (node->dest_ - node->pos_) / t_last;
        }

        Vector R;       // the gaussian variable
        Vector v;       // the new velocity
        Vector dest;    // the new destination
        model_time_t t; // duration of the movement

        bool done = false;
        while (!done) {
            // get R
            R.x_ = getRandomDouble(DIST_GAUSS, 0, SIGMA_);
            R.y_ = getRandomDouble(DIST_GAUSS, 0, SIGMA_);

            // apply the formula to get the new velocity
            v = v_last * ALPHA_ + R * A_;

            // the new destination
            dest = node->dest_ + v * T_;

            if (area_->inside(dest)) {
                t = T_;
                done = true;
            } else {
                area_->findNearestPoint(dest);
                if (! (dest == node->dest_)) {
                    t = (dest - node->dest_).length() / v.length();
                    done = true;
                }
            }
        }

        // update the node
        node->pos_ = node->dest_;
        node->dest_ = dest;
        node->startTime_ = node->nextStartTime_;
        node->arrivalTime_ = node->startTime_ + t;
        node->nextStartTime_ = node->startTime_ + t;
        node->speed_ = v.length();
    }
}
