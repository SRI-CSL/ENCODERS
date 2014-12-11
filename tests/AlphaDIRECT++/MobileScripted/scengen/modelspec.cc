//
// modelspec.cc
//

#include "defs.h"

#include "util.h"
#include "model.h"
#include "waypoint.h"
#include "fixedwp.h"
#include "brownian.h"
#include "pursue.h"
#include "column.h"
#include "gaussmarkov.h"

#include "modelspec.h"

ModelSpec *ModelSpec::instance_ = NULL;

// Constructor
ModelSpec::ModelSpec() : Spec()
{
    modelList_ = new List();
    assert(modelList_ != NULL);
}

// Destructor
ModelSpec::~ModelSpec()
{
    assert(modelList_ != NULL);

    modelList_->reset();
    List *model = NULL;
    while ((model = (List *)(modelList_->nextValue())) != NULL) {
        model->clear();
    }

    modelList_->clear();
    delete modelList_;
}

ModelSpec *ModelSpec::instance()
{
    if (instance_ == NULL) {
        instance_ = new ModelSpec();
        assert(instance_);
    }
    return instance_;
}

// read a model specs file
int ModelSpec::load(char *filename)
{
    assert(openFile(filename));

    Model *model = NULL;
    char modelName[MAX_SPEC_LINE];
    char key[MAX_SPEC_LINE];
    char value[MAX_SPEC_LINE];

    char *line;
    while ( (line = getLine()) != NULL) {
        if (line[0] == '[') {    // starts a new model
            if (model != NULL) {    // if previously we have a model
                                    // insert it to the list
                assert(modelName != NULL);
                modelList_->set(modelName, (void *)model);
                bzero(modelName, MAX_SPEC_LINE);
                model = NULL;
            }

            bzero(modelName, MAX_SPEC_LINE);
            sscanf(line, "[ %s ]", modelName);

            model = createModelByName(modelName);

            //fprintf(stderr, "modelName: %s in %s\n", modelName, line);

        } else {
            if (model == NULL) {
                fprintf(stderr, "Parameter not specified within a section?\n");
                exit(1);
            }
            // we now have a "key = value" pair here
            sscanf(line, "%s = %s", key, value);
            model->set(key, value);
        }
    }

    // count for the last model
    if (model != NULL) {    
        assert(modelName != NULL);
        modelList_->set(modelName, (void *)model);
    }

    return 1;
}

// get access of a model
Model *ModelSpec::getModel(char *modelName)
{
    //printf("Getting %s from modelspec\n", modelName);
    if (modelList_ == NULL) {
        assert(modelList_ != NULL);
        return NULL;
    } else {
        return (Model *)(modelList_->get(modelName));
    }
}

// create a copy of a model
Model *ModelSpec::createModel(char *modelName, long seed)
{
    //printf("Getting %s from modelspec\n", modelName);
    if (modelList_ == NULL) {
        assert(modelList_ != NULL);
        return NULL;
    }

    Model *model = createModelByName(modelName, seed);
    getModel(modelName)->copyParam(model);

    return model;
}

Model *ModelSpec::createModelByName(char *modelName, long seed)
{
    Model *model = NULL;

    if (strcmp(modelName, "Waypoint") == 0) {
        model = new Waypoint(seed);
    } else if (strcmp(modelName, "FixedWP") == 0) {
        model = new FixedWP(seed);
    } else if (! strcmp(modelName, "Brownian")){
        model = new Brownian(seed);
    } else if (! strcmp(modelName, "Pursue")) {
        model = new Pursue(seed);
    } else if (! strcmp(modelName, "Column")) {
        model = new Column(seed);
    } else if (! strcmp(modelName, "Gauss-Markov")) {
        model = new GaussMarkov(seed);
    } else {
        // new models should be added here
        fprintf(stderr, "Model %s not supported\n", modelName);
        exit (1);
    }
    assert (model != NULL);

    return model;
}
