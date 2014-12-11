//
// model.cc
//

#include "defs.h"

#include "random.h"
#include "util.h"
#include "model.h"
#include "nodeman.h"

// class Area

// Constructor
Area::Area(char *areaType, long seed)
{
    // areaType should be something like "rect(100,200)" or "circle(150)"
    assert(areaType != NULL);

    if (!strncmp(areaType, "rect", 4)) {    // Rectangle
        type_ = AREA_RECT;
        // parameters should be "rect(X,Y)"
        params_ = new double[2];
        sscanf(areaType, "rect(%lf,%lf)", &(params_[0]), &(params_[1]));
    } else {
        type_ = AREA_UNKNOWN;
        params_ = NULL;
    }

    seed_ = seed;
}

// Destructor
Area::~Area()
{
    if (params_ != NULL)
        delete params_;
}

// Member functions of class Area
void Area::randomPos(Vector &v)
{
    if (type_ == AREA_RECT) {
        v.x_ = UNIFORM(seed_) * params_[0];
        v.y_ = UNIFORM(seed_) * params_[1];
    } else {
        assert(0);
    }
}

void Area::center(Vector &v)
{
    if (type_ == AREA_RECT) {
        v.x_ = params_[0] / 2;
        v.y_ = params_[1] / 2;
    } else {
        assert(0);
    }
}

double Area::maxX()
{
    if (type_ == AREA_RECT) {
        return params_[0];
    } else {
        assert(0);
        return -1; 
    }
}

double Area::maxY()
{
    if (type_ == AREA_RECT) {
        return params_[1];
    } else {
        assert(0);
        return -1;
    }
}

bool Area::inside(Vector &p)
{
    if (type_ == AREA_RECT) {
        if (p.x_ < 0 || p.x_ > params_[0] || \
            p.y_ < 0 || p.y_ > params_[1]) {
            return false;
        } else {
            return true;
        }
    } else {
        assert (0);
        return false;
    }
}

void Area::findNearestPoint(Vector &p)
{
    if (inside(p)) return;

    if (type_ == AREA_RECT) {
        if (p.x_ < 0) p.x_ = 0;
        else if (p.x_ > params_[0]) p.x_ = params_[0];

        if (p.y_ < 0) p.y_ = 0;
        else if (p.y_ > params_[1]) p.y_ = params_[1];
    } else {
        assert(0);
    }
}

// class Model

// Constructor
Model::Model(long seed)
{
    type_ = MODEL_UNKNOWN;
    paramList_ = new List();
    assert(paramList_ != NULL);

    startTime_ = 0.0;
    startID_ = 0;
    numNodes_ = 0;
    area_ = NULL;

    nodes_ = NULL;

    enableAcceleration_ = false;
    ACC_T_ = 0;
    ACC_A_ = 0;
    shadowNodes_ = NULL;

    initialized_ = false;

    seed_ = seed;
}

// Destructor
Model::~Model()
{
    assert(paramList_ != NULL);
    delete paramList_;

    if (area_ != NULL)
        delete area_;

    if (shadowNodes_ != NULL) {
        delete shadowNodes_;
    }
}

// Member functions

// Get the string value of a given parameter name
char *Model::get(char *key)
{
    return (char *)(paramList_->get(key));
}

int Model::geti(char *key)
{
    return paramList_->geti(key);
}

long Model::getl(char *key)
{
    return paramList_->getl(key);
}

double Model::getf(char *key)
{
    return paramList_->getf(key);
}

// Set the string value of a given parameter
int Model::set(char *key, char *value)
{
    char *v = Util::cloneStr(value);
    return paramList_->set(key, v);
}

// Make a copy of the parameters of the current model object
void Model::copyParam(Model *target)
{
    char *key, *value;
    paramList_->reset();
    while ( (key = paramList_->nextKey()) != NULL) {
        value = Util::cloneStr((char *)(paramList_->get(key)));
        target->set(key, value);
    }
}

// get the type of the model
ModelType Model::type()
{
    return type_;
}

// get access to a node
Node *Model::getNode(node_id_t id)
{
    if (nodes_ == NULL || id < startID_ || id > startID_ + numNodes_) {
        return NULL;
    }
    return &(nodes_[id - startID_]);
}

// dump the contents of the paramlist
void Model::dump()
{
    if (paramList_ == NULL)
        return;

    char *key, *value;
    paramList_->reset();
    while ( (key = paramList_->nextKey()) != NULL) {
        value = (char *)(paramList_->get(key));
        printf("%s => %s\n", key, value);
        fflush(stdout);
    }
}

// get the type of distribution of the random variable by name
void Model::getDistType(char *typeName, DistType &type)
{
    assert(typeName != NULL);
    if (!strcmp(typeName, "constant")) {
        type = DIST_CONST;
    } else if (!strcmp(typeName, "uniform")) {
        type = DIST_UNIFORM;
    } else if (!strcmp(typeName, "gauss")) {
        type = DIST_GAUSS;
    } else {
        type = DIST_UNKNOWN;
    }
}

// get a double value by giving 2 parameters and the type of distribution
double Model::getRandomDouble(DistType type, double param1, double param2)
{
    if (type == DIST_CONST) {
        return param1;
    } else if (type == DIST_UNIFORM) {
        double min, max;
        if (param1 < param2) {
            min = param1;
            max = param2;
        } else {
            min = param2;
            max = param1;
        }
        return min + (max-min)*UNIFORM(seed_);
    } else if (type == DIST_GAUSS) {
        double mean = param1;
        double variance = param2;
        return (GAUSS(seed_) * variance + mean);
    } else {
        // shouldn't come here
        assert(0);
        return 0;
    }
}


// initialize model parameters
void Model::init(model_time_t startTime, \
                 model_time_t stopTime, \
                 node_id_t startID, \
                 int numNodes, \
                 Area *area, \
                 bool cp)
{
    assert (startTime >= 0 && stopTime > startTime);

    startTime_ = startTime;
    stopTime_ = stopTime;
    startID_ = startID;
    if (cp) {
        numNodes_ = 1;
    } else {
        numNodes_ = numNodes;
    }

    area_ = area;
    assert(area_ != NULL);

    assert(startID_ >= 0);
    assert(numNodes_ > 0 && numNodes_ <= MAX_NODES);

    if (cp) {
        nodes_ = new Node();
        assert(nodes_);
        bzero(nodes_, sizeof(Node));
    } else {
        // allocate the nodes from Node Manager
        nodes_ = NodeMan::instance()->allocNode(numNodes_);
    }

    // init the nodes
    initNodes(nodes_, numNodes_);

    if (paramList_->get("ACC_T") && paramList_->get("ACC_A")) {
        // if these two parameters are set
        // enable acceleration
        enableAcceleration_ = true;

        // and init shadow nodes
        createShadow();
    }
}

void Model::createShadow()
{
    ACC_T_ = getf("ACC_T");
    ACC_A_ = getf("ACC_A");

    assert(ACC_T_ > 0 && ACC_A_ > 0);

    shadowNodes_ = new Node[numNodes_];
    assert(shadowNodes_);
    initNodes(shadowNodes_, numNodes_);

    pendingMoves_ = new List *[numNodes_];
    assert(pendingMoves_);
    for (int i=0; i<numNodes_; i++) {
        pendingMoves_[i] = new List(true);
        assert(pendingMoves_[i]);
    }
}

void Model::initNodes(Node *nodes, int numNodes)
{
    for (int i=0; i<numNodes; i++) {
        nodes[i].id_ = i+startID_;
        nodes[i].startTime_ = startTime_;
        nodes[i].arrivalTime_ = startTime_;
        nodes[i].nextStartTime_ = startTime_;
        nodes[i].pos_.x_ = -1.0;
        nodes[i].pos_.y_ = -1.0;
        nodes[i].dest_.x_ = -1.0;
        nodes[i].dest_.y_ = -1.0;
        nodes[i].speed_ = 0.0;
        nodes[i].numMoves_ = 0;
    }
}

model_time_t Model::nextMove(Move *move, bool probeOnly)
{
    assert(numNodes_ > 0 && initialized_);
    
    model_time_t min_time = stopTime_ + 1;

    int min_node = findMinNode(min_time);

    if (min_time > stopTime_) {     // simulation stopped
        //min_time = -1;
        move->time_ = -1;
    } else {
        if (probeOnly) {
            //move->time_ = min_time; // why this causes problem?
            // sometimes min_time != nodes_[min_node].nextStartTime_...
            move->time_ = nodes_[min_node].nextStartTime_;
            move->id_ = nodes_[min_node].id_;
            move->firstMove_ = (nodes_[min_node].numMoves_ <= 1);
        } else {
            Node *node = &(nodes_[min_node]);
            
            if (enableAcceleration_) {
                makeAcceleratedMove(node);
            } else {
                makeMove(node);
                node->numMoves_++;
            }

            // create move
            move->time_ = node->startTime_;
            move->id_ = node->id_;
            move->dest_ = node->dest_;
            move->speed_ = node->speed_;
            move->firstMove_ = (nodes_[min_node].numMoves_ <= 1);
            move->arrivalTime_ = node->arrivalTime_;
            move->nextStartTime_ = node->nextStartTime_;
        }
    }

    //return min_time;
    return move->time_;
}

// find the node with min next start time
// returns the index of the node, minTime has the min time
int Model::findMinNode(model_time_t &minTime)
{
    minTime = stopTime_ + 1;
    int index = -1;

    for (int i=0; i<numNodes_; i++) {
        if (nodes_[i].nextStartTime_ < minTime) {
            minTime = nodes_[i].nextStartTime_;
            index = i;
        }
    }

    return index;
}

void Model::makeAcceleratedMove(Node *node)
{
    // get the shadow node
    int shadowIndex = node->id_ - startID_;
    Node *sn = &(shadowNodes_[shadowIndex]);

    // Note: moves are stored as struct Node in the pending move lists
    //       we rely on the iterator of the List class
    List *pList = pendingMoves_[shadowIndex];
    Node *n = (Node *) (pList->nextValue());

    // if pending moves empty
    if (n == NULL) {
        // update shadow
        makeMove(sn);
        sn->numMoves_++;

        // generate pending moves
        pList->clear();
        makePendingMoves(pList, sn);

        // get the first move
        pList->reset();
        n = (Node *) (pList->nextValue());
        assert(n);
    }

    // copy next move from pending moves
    memcpy(node, n, sizeof(Node));
}

void Model::makePendingMoves(List *pList, Node *sn)
{
    //if (sn->speed_ == 0) {      
    if (sn->numMoves_ <= 1) {      
        // first move, set position only, so just simply copy the move
        Node *n = new Node();
        assert(n);
        memcpy(n, sn, sizeof(Node));
        pList->set("dummy_key", n);
        return;
    } 

    // normal moves

    // the distance of the movement
    double d = (sn->dest_ - sn->pos_).length();

    // uniformed (has length of 1) velocity vector 
    Vector v = (sn->dest_ - sn->pos_) / d;

    // the max speed that can be achived with ACC_A_
    double v_max = sqrt(ACC_A_ * d);

    // record the pause time of the shadow move
    model_time_t pause = sn->nextStartTime_ - sn->arrivalTime_;

    // the time when node moves with constant (max) speed
    model_time_t c_time = 0;

    if (sn->speed_ > v_max) {   // unreachable speed
        // reduce it to v_max
        sn->speed_ = v_max;
        c_time = 0;
    } else {
        // make v_max consistent with sn->speed_
        v_max = sn->speed_;

        // the distance covered in acceleration phase
        double d1 = v_max * v_max / ACC_A_;
        // the distance covered in constant speed phase
        double d2 = d - d1;

        c_time = d2 / v_max;
    }

    // speed increment in one time interval
    double unit_a = ACC_A_ * ACC_T_;

    // phase of movement
    // 1 == positive acceleration
    // 0 == constant speed
    // -1 == negative acceleratoin
    double phase = 1;

    // keep track of the simulation time of the movement
    model_time_t s_time = sn->startTime_;

    // keep track of current position
    Vector pos = sn->pos_;

    // keep track of the speed
    double speed = 0;

    bool done = false;
    while (!done) {
        Node *n = new Node();
        n->numMoves_ = sn->numMoves_;
        assert(n);

        double end_speed;
        double average_speed;
        model_time_t move_time;
        // distance covered
        double move_d;

        if (phase == 1) {           // positive acceleration phase
            if (speed < v_max - unit_a) {
                move_time = ACC_T_;
                end_speed = speed + unit_a;
            } else {
                move_time = (v_max - speed) / ACC_A_;
                end_speed = v_max;

                // shift to constant move
                phase = 0;
            }
        } else if (phase == -1) {   // negative acceleration phase
            if (speed > unit_a) {
                move_time = ACC_T_;
                end_speed = speed - unit_a;
            } else {
                move_time = speed / ACC_A_;
                end_speed = 0;

                // movements ends here
                done = true;
            }
        } else {                // constant speed phase
            end_speed = speed;
            move_time = c_time;

            // shift to negative acceleration phase
            phase = -1;
        }

        average_speed = (end_speed + speed) / 2;
        move_d = average_speed * move_time;

        // create the move
        n->id_ = sn->id_;
        n->startTime_ = s_time;
        n->arrivalTime_ = n->startTime_ + move_time;
        if (done) {
            // only count for pause time in the last move
            n->nextStartTime_ = n->arrivalTime_ + pause;
        } else {
            n->nextStartTime_ = n->arrivalTime_;
        }
        n->speed_ = average_speed;
        n->pos_ = pos;
        n->dest_ = pos + v * move_d;

        // add it to pList
        pList->set("dummy_key", n);

        // update variables
        s_time = n->nextStartTime_;
        speed = end_speed;
        pos = n->dest_;
    }

    // modify shadow node, so that the simulation times are consistent
    sn->nextStartTime_ = s_time;
    sn->arrivalTime_ = s_time - pause;
}

// make the normal movement.
void Model::makeMove(Node *node)
{
    // should be overloaded by sub classes
    assert(0);
}

