//
// scenspec.h
//

#ifndef _scenspec_
#define _scenspec_

#include "util.h"
#include "model.h"
#include "spec.h"
#include "nodegroup.h"

class ScenSpec : public Spec
{
public:
    ScenSpec();
    ~ScenSpec();

    // initialize all node groups
    void init();
    model_time_t nextMove(Move *);

    int load(char *filename);
    //NodeGroup *getGroup(char *groupName);

    void dump();

    // global specs
    long seed_;
    int numNodes_;
    Area *area_;
    model_time_t startTime_;
    model_time_t stopTime_;

    bool print_dest_comments_;

protected:
    List *groupList_;
    int groupListSize_;

    // used in move generation
    bool initialized_;
    Move **moveList_;               // array of moves from all groups
    model_time_t *startTimeList_;   // array of start times
    NodeGroup **groupArray_;
};

#endif
