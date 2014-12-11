//
// gaussmarkov.h
//
// Gauss-Markov Model
//

#ifndef _gaussmarkov_
#define _gaussmarkov_

#include "random.h"
#include "model.h"

class GaussMarkov : public Model
{
public:
    GaussMarkov(long seed = 0);
    ~GaussMarkov();

    void init(model_time_t startTime = 0.0, \
              model_time_t stopTime = 0.0, \
              node_id_t startID = 0, \
              int num_nodes = 0, \
              Area *area = NULL, \
              bool cp = false);

protected:
    void makeMove(Node *node);

    // The time interval
    model_time_t T_;
    
    // The standard deviation sigma
    double SIGMA_;

    // The factor alpha
    double ALPHA_;

    // The gain applied to the gaussian process
    double A_;
};

#endif
