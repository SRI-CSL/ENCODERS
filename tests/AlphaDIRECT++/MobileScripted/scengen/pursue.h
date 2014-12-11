//
// pursue.h
//
// Pursue Motion Model
//

#ifndef _pursue_
#define _pursue_

#include "model.h"

class Pursue : public Model
{
public:
    Pursue(long seed = 0);
    ~Pursue();

    void init(model_time_t startTime = 0.0, \
              model_time_t stopTime = 0.0, \
              node_id_t startID = 0, \
              int num_nodes = 0, \
              Area *area = NULL, \
              bool cp = false);
protected:
    void makeMove(Node *node);

    // the node being chased
    Node *thief_;
    // max and min speed of the "thief"
    // assume uniform distribution
    double V_min_;
    double V_max_;
    // pause time
    model_time_t T_;

    // the "pursuing" nodes
    Node *police_;
    // max and min speed of the "police"
    // assume uniform distribution
    double V_pmin_;
    double V_pmax_;
};

#endif
