//
// column.h
//
// Column Motion Model
//

#ifndef _column_
#define _column_

#include "model.h"

class Column : public Model
{
public:
    Column(long seed = 0);
    ~Column();

    void init(model_time_t startTime = 0.0, \
              model_time_t stopTime = 0.0, \
              node_id_t startID = 0, \
              int num_nodes = 0, \
              Area *area = NULL, \
              bool cp = false);

protected:
    void makeMove(Node *node);

    // min and max speed of the nodes
    double V_min_, V_max_;

    // time interval
    model_time_t T_;

    // angle of the velocity, in terms of multiples of PI
    double A_;
};

#endif
