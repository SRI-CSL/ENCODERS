//
// modelspec.h
//


#ifndef _modelspec_
#define _modelspec_

#include "util.h"
#include "spec.h"
#include "model.h"

class ModelSpec : public Spec
{
public:
    ~ModelSpec();
    static ModelSpec *instance();

    int load(char *filename);
    Model *getModel(char *modelName);
    Model *createModel(char *modelName, long seed = 0);

protected:
    ModelSpec();
    Model *createModelByName(char *modelName, long seed = 0);

    static ModelSpec *instance_;
    List *modelList_;
};

#endif
