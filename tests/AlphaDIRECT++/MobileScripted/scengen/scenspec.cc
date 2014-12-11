//
// scenspec.cc
//

#include "defs.h"

#include "nodegroup.h"
#include "modelspec.h"
#include "scenspec.h"
#include "nodeman.h"

// Constructor
ScenSpec::ScenSpec()
{
    groupList_ = new List();
    assert(groupList_ != NULL);
    groupListSize_ = 0;
    //globalGroup_ = NULL;

    moveList_ = NULL;
    startTimeList_ = NULL;
    groupArray_ = NULL;
    initialized_ = false;

    numNodes_ = 0;
    area_ = NULL;
    startTime_ = 0.0;
    stopTime_ = 0.0;
    seed_ = 0;
    print_dest_comments_ = false;
}

// Destructor
ScenSpec::~ScenSpec()
{
    assert(groupList_ != NULL);

    groupList_->reset();
    NodeGroup *group = NULL;
    while ( (group = (NodeGroup *)(groupList_->nextValue())) != NULL) {
        delete group;
    }

    delete groupList_;
}

// Member functions

// load scenario spec file
int ScenSpec::load(char *filename)
{
    assert(openFile(filename));

    NodeGroup *group = NULL;
    char groupName[MAX_SPEC_LINE];
    char key[MAX_SPEC_LINE];
    char value[MAX_SPEC_LINE];
    char tmp[MAX_SPEC_LINE];
    bool global = false;

    char *line;
    while ( (line = getLine()) != NULL) {
        if (line[0] == '[') {    // starts a new group
            if (group != NULL) {    // if previously we have a group
                                    // insert it to the list
                assert(groupName != NULL);
                groupList_->set(groupName, (void *)group);
                bzero(groupName, MAX_SPEC_LINE);
                group = NULL;
            }

            // get the name of the new group
            bzero(groupName, MAX_SPEC_LINE);
            sscanf(line, "[ %s ]", groupName);

            // is that a global spec?
            if (!strcmp(groupName, "Global") ||
                !strcmp(groupName, "global") ) {
                global = true;
            } else {
                global = false;
                // create new group
                group = new NodeGroup(seed_);
                assert (group);
            }

            //fprintf(stderr, "modelName: %s in %s\n", modelName, line);

        } else {                // set parameters of the group
            assert (global || group);

            // we now have a "key = value" pair here
            sscanf(line, "%s = %s", key, value);

            if (!strcmp(key, "seed")) {
                long n = atol(value);
                if (global) {
                    seed_ = n;
                } 
            } else if (!strcmp(key, "print_dest_comments")) {
                long n = atol(value);
                if (n == 1) {
                    print_dest_comments_ = true;
                }
            } else if (!strcmp(key, "num_nodes")) {
            // set the value for the group
                int n = atoi(value);
                if (global) {
                    //numNodes_ = n;
                } else {
                    group->setParam(key, &n);
                    numNodes_ += n;
                }
            } else if (!strcmp(key, "area")) {
                assert(global);
                area_ = new Area((char *)value, seed_);
            } else if (!strcmp(key, "center_area") || \
                       !strcmp(key, "member_area") || \
                       !strcmp(key, "center_model") || \
                       !strcmp(key, "member_model")) {
                assert(group);
                group->setParam(key, value);
            } else if (!strcmp(key, "offset")) {
                double x,y;
                sscanf(value, "%lf,%lf", &x, &y);
                assert(group);
                group->setParam("offset", &x, &y);
            } else if (!strcmp(key, "start_time")) {
                model_time_t t;
                sscanf(value, "%lf", &t);

                if (global) {
                    startTime_ = t;
                } else {
                    group->setParam(key, &t);
                }
            } else if (!strcmp(key, "stop_time")) {
                model_time_t t;
                sscanf(value, "%lf", &t);

                if (global) {
                    stopTime_ = t;
                } else {
                    group->setParam(key, &t);
                }
            } else if (strstr(key, "center_model.") != NULL) {
                assert(group);
                sscanf(key, "center_model.%s", tmp);
                group->setParam("center_model.", tmp, value);
            } else if (strstr(key, "member_model.") != NULL) {
                assert(group);
                sscanf(key, "member_model.%s", tmp);
                group->setParam("member_model.", tmp, value);
            } else {
                fprintf(stderr, "Unknown parameter %s\n", key);
                assert(0);
            }
        } // if (line[0] == '[') 
    } // while

    // count for the last group
    if (group != NULL) {    
        assert(groupName != NULL);
        groupList_->set(groupName, (void *)group);
    }

    // check the global section
    assert(numNodes_>0 && area_  && startTime_>=0 && stopTime_>startTime_);

    return 1;
}

// dump the contents of group list
void ScenSpec::dump()
{
    char *key;
    NodeGroup *g;

    groupList_->reset();
    while ( (key = groupList_->nextKey()) != NULL) {
        g = (NodeGroup *)(groupList_->get(key));
        printf("\nGroup %s:\n", key);
        g->dump();
    } // while
}

// initialize (including the models in) all groups
void ScenSpec::init()
{
    assert(groupList_ != NULL);

    // initialized the node manager first
    NodeMan::instance()->init(numNodes_);

    // prepare for the move generation

    // total number of groups except for the global one
    groupListSize_ = groupList_->getCount();
    //printf("groupListSize_ = %d\n", groupListSize_);

    // create arrays that store info of moves
    moveList_ = new Move *[groupListSize_];
    startTimeList_ = new model_time_t[groupListSize_];
    //modelList_ = new Model *[groupListSize_];
    groupArray_ = new NodeGroup *[groupListSize_];

    for (int i=0; i<groupListSize_; i++) {
        moveList_[i] = new Move();
        assert(moveList_[i]);
        bzero(moveList_[i], sizeof(Move));
    } // for

    // init all the groups
    NodeGroup *g = NULL;
    groupList_->reset();
    node_id_t id = 1;
    int i=0;

    while ( (g = (NodeGroup *)groupList_->nextValue()) != NULL) {
        // init the group
        id += g->init(id, startTime_, stopTime_, area_);

        // store the pointers of the groups into an array
        groupArray_[i] = g;
        i++;
    } // while

    // generate the first moves of the models 
    // and store the info into moveList_ and startTimeList_
    for (i=0; i<groupListSize_; i++) {
        startTimeList_[i] = groupArray_[i]->nextMove(moveList_[i]);
    }

    initialized_ = true;
}

// generate the next movement
model_time_t ScenSpec::nextMove(Move *move)
{
    assert(initialized_);

    model_time_t min_time = stopTime_ + 1;
    int min_index = 0;

    // find the group/model with minimum start time
    for (int i=0; i<groupListSize_; i++) {
        if ( startTimeList_[i] >= 0 && \
             startTimeList_[i] < min_time) {
            min_index = i;
            min_time = startTimeList_[i];
        }
    } // for

    if (min_time < 0 || min_time > stopTime_) {  // simulation stopped
        return -1;
    } else {
        // copy the move out
        memcpy(move, moveList_[min_index], sizeof(Move));

        // and update the model and move list
        startTimeList_[min_index] = 
            groupArray_[min_index]->nextMove(moveList_[min_index]);

        return min_time;
    } // if
}

