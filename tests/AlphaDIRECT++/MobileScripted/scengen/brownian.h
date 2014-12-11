//
// brownian.h
//
// Brownian Motion Model
//

#ifndef _brownian_
#define _brownian_

#include "model.h"
#include "random.h"

class Brownian : public Model
{
public:
    Brownian(long seed = 0);
    ~Brownian();

    void init(model_time_t startTime = 0.0, \
              model_time_t stopTime = 0.0, \
              node_id_t startID = 0, \
              int num_nodes = 0, \
              Area *area = NULL, \
              bool cp = false);

protected:
    // construct the move
    void makeMove(Node *node);

    // movement interval duration
    model_time_t T_;

    // assumptions: V is uniformly distributed from (0, V_)
    // maximum speed
    double V_;

    // the min and max angle that we choose from at each time interval
    // in radians
    // assume uniform distribution
    double A_min_, A_max_;
};

#endif
