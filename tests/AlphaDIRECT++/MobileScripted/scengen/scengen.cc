//
// scengen.cc
//

#include "defs.h"

#include "util.h"
#include "spec.h"
#include "modelspec.h"
#include "scenspec.h"
#include "model.h"
#include "random.h"
#include "nodeman.h"
#include "scengen.h"

// the main function
int main()
{
    ScenGen *sg = ScenGen::instance();

    Move *move = new Move();
    model_time_t time = 0;

    sg->printComments();
    while ( (time = sg->nextMove(move)) >= 0) {
        sg->formatMove(move);
    }// while

    return 0;
}

// Implementation of class ScenGen (singleton)

// static member variable
ScenGen *ScenGen::instance_ = NULL;

// constructor and destructor
ScenGen::ScenGen()
{
    // Note: the order of initialization is important here

    // load the model specifications
    modelSpec_ = ModelSpec::instance();
    assert(modelSpec_);

    modelSpec_->load(MODEL_FILE);

    // load the scenario specifications
    scenSpec_ = new ScenSpec();
    assert(scenSpec_);

    scenSpec_->load(SCEN_FILE);
    print_dest_comments_ = scenSpec_->print_dest_comments_;

    // initialize the scenario
    scenSpec_->init();

    nm_ = NodeMan::instance();
}

ScenGen::~ScenGen()
{
    assert(modelSpec_);
    delete modelSpec_;

    assert(scenSpec_);
    delete scenSpec_;
}

// method to get the instance 
ScenGen *ScenGen::instance()
{
    if (instance_ == NULL) {
        instance_ = new ScenGen();
        assert (instance_ != NULL);
    }
    return instance_;
}

// generate one move at a time
model_time_t ScenGen::nextMove(Move *move)
{
    assert(modelSpec_ && scenSpec_);
    return scenSpec_->nextMove(move);
}

// format a movement to the scenario file format
void ScenGen::formatMove(Move *move)
{
    //if (move->speed_ == 0) {
    if (move->firstMove_) {
        // initial move, set position only
        printf(SETPOS_FORMAT, move->id_, "X_", move->dest_.x_);
        printf(SETPOS_FORMAT, move->id_, "Y_", move->dest_.y_);
        printf(SETPOS_FORMAT, move->id_, "Z_", 0.0);
    } else {
        if (move->speed_ == 0) {
            // effectively not moving
            /*
            cerr << id << ": at " << move->time_ 
                << " to " << move->dest_.x_ << ", " << move->dest_.y_ << endl;
                */
        } else {
            // velocity, pause arrivalTime_, nextStartTime_
            // following moves
            if (print_dest_comments_) {
                printf(SETDEST_COMMENT_FORMAT, \
                    move->speed_, \
                    move->nextStartTime_ - move->arrivalTime_);
            }
            printf(SETDEST_FORMAT, \
                    move->time_, \
                    move->id_, \
                    move->dest_.x_, \
                    move->dest_.y_, \
                    move->speed_);
        }
    }
}

// print the comments, which are neccessary for ad-hockey
void ScenGen::printComments()
{
    // print out the comment
    printf(COMMENT_FORMAT, \
           scenSpec_->numNodes_, \
           scenSpec_->stopTime_, \
           scenSpec_->area_->maxX(), \
           scenSpec_->area_->maxY());
}
