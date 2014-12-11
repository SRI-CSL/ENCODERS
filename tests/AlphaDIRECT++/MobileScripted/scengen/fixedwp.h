//
// fixedwp.h
//

#ifndef _fixedwp_
#define _fixedwp_

class FixedWP : public Model
{
public:
    FixedWP(long seed = 0);
    ~FixedWP();

    void init(model_time_t startTime = 0, \
              model_time_t stopTime = 0, \
              node_id_t startID = 0, \
              int num_nodes = 0, \
              Area *area = NULL, \
              bool cp = false);

    //model_time_t nextMove(Move *move, bool probeOnly = false);

protected:
    void makeMove(Node *node);

    // list of precomputed Moves
    List *moveList_;

    bool firstMove_;    // whether the current move is the first
                        // since the movements can be circular

    //model_time_t moveTime_; // start time for current movement

    Move *nextMove_;    // one move ahead
};

#endif
