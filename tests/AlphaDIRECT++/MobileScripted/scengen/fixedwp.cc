//
// fixedwp.cc
//
// Fixed Waypoint Model

#include "defs.h"

#include "util.h"
#include "model.h"
#include "fixedwp.h"

FixedWP::FixedWP(long seed) : Model(seed)
{
    type_ = MODEL_FIXEDWP;

    moveList_ = new List();
    assert(moveList_);
    firstMove_ = true;
    nextMove_ = NULL;
}

FixedWP::~FixedWP()
{
    assert(moveList_);
    delete moveList_;
}

void FixedWP::init(model_time_t startTime, model_time_t stopTime, \
                   node_id_t startID, int num_nodes, Area *area, bool cp)
{
    // it doesn't make sense if 2 nodes move in exactly the same way
    assert(num_nodes == 1);

    Model::init(startTime, stopTime, startID, num_nodes, area, cp);

    // convert the strings in paramList_ to Move structs in moveList_
    assert(paramList_->getCount() > 0);

    long i = 0;
    char key[100];   // used to construct the key in the list
                     // the constant here shouldn't be problematic

    char *move_s = NULL;
    Move *move = NULL;
    double destX, destY, speed, pause;

    paramList_->reset();

    while ( (move_s = (char *)paramList_->nextValue()) != NULL) {
        int result = sscanf(move_s, "(%lf,%lf,%lf,%lf)", \
                            &destX, &destY, &speed, &pause);
        if (result == 0) continue;

        move = new Move();
        assert(move != NULL);

        // since this model is just for one node
        move->id_ = startID_;
        move->dest_.x_ = destX;
        move->dest_.y_ = destY;
        move->speed_ = speed;

        // set the time of move to the pause time
        // the correct time should be calculated in nextMove()
        move->time_ = pause;

        // insert the move to the moveList_
        i++;
        sprintf(key, "wp%ld", i);

        //cerr << "Adding move " << key << ": " << move->id_ << endl;
        moveList_->set(key, move);

    } // while

    // should at least have one move
    assert(moveList_->getCount() > 0);

    // get the first move as the nextMove_
    moveList_->reset();
    nextMove_ = (Move *) (moveList_->nextValue());
    assert(nextMove_);

    firstMove_ = true;
    initialized_ = true;
}

// create movement
void FixedWP::makeMove(Node *node)
{
    assert(node);

    if (firstMove_) {   // first move
        node->pos_ = nextMove_->dest_;
        node->dest_ = nextMove_->dest_;
        node->nextStartTime_ = startTime_ + nextMove_->time_;
        node->speed_ = 0;
        firstMove_ = false;
    } else {
        node->pos_ = node->dest_;
        node->dest_ = nextMove_->dest_;
        node->speed_ = nextMove_->speed_;
        node->startTime_ = node->nextStartTime_;
        // time it takes to move to the dest
        model_time_t t = (node->dest_ - node->pos_).length()/node->speed_;
        node->arrivalTime_ = node->startTime_ + t;
        node->nextStartTime_ = node->arrivalTime_ + nextMove_->time_;
    }


    // then advance nextMove_
    nextMove_ = (Move *) (moveList_->nextValue());
    if (nextMove_ == NULL) {
        moveList_->reset();
        nextMove_ = (Move *) (moveList_->nextValue());
    }
    assert(nextMove_);

    //fprintf(stderr, "FixedWP: making move: id:%ld, time:%f, speed:%f\n",
    //        node->id_, node->startTime_, node->speed_);
}
