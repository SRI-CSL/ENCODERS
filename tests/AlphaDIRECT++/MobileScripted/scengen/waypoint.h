//
// waypoint.h
//

#ifndef _waypoint_
#define _waypoint_

#include "random.h"
#include "model.h"

class Waypoint : public Model
{
public:
    Waypoint(long seed = 0);
    ~Waypoint();

    void init(model_time_t startTime = 0.0, \
              model_time_t stopTime = 0.0, \
              node_id_t startID = 0, \
              int num_nodes = 0, \
              Area *area = NULL, \
              bool cp = false);

protected:
    void makeMove(Node *node);

    model_time_t T_max_;
    model_time_t T_min_;
    DistType T_dist_;
    
    double V_max_;
    double V_min_;
    DistType V_dist_;
    
    bool randomPos_;
};

#endif
