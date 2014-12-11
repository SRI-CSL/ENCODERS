//
// scengen.h
//

#ifndef _scengen_
#define _scengen_

#include "model.h"
#include "modelspec.h"
#include "scenspec.h"

#define MODEL_FILE "model-spec"
#define SCEN_FILE "scen-spec"

// scenario file format for NS2
// output is directed to stdout
#define SETDEST_COMMENT_FORMAT "## velocity: %.12f, pause: %.12f\n" 
#define SETDEST_FORMAT "$ns_ at %.12f \"$node_(%ld) setdest %.12f %.12f %.12f\"\n"
#define SETPOS_FORMAT "$node_(%ld) set %s %.12f\n"

// certain comments are neccessary for ad-hockey to run correctly
#define COMMENT_FORMAT "# nodes: %d, max time: %.2f, max x: %.2f, max y: %.2f\n"

class ScenGen
{
public:
    ~ScenGen();
    static ScenGen *instance();

    model_time_t nextMove(Move *move);
    void formatMove(Move *move);
    void printComments();

protected:
    ScenGen();
    static ScenGen *instance_;

    ModelSpec *modelSpec_;
    ScenSpec *scenSpec_;
    NodeMan *nm_;

    bool print_dest_comments_;
};

#endif
