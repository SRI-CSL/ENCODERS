//
// model.h
//

#ifndef _model_
#define _model_

#include <math.h>

#include "random.h"
#include "util.h"

#define INFINITESIMAL 1e-8

#define MAX_NODES 1000

typedef double model_time_t;
typedef unsigned long node_id_t;

class Vector
{
public:
    Vector(double x = 0.0, double y = 0.0) {
        x_ = x; y_ = y;
    };

    double length() {
        return sqrt(x_*x_ + y_*y_);
    };

    void operator=(Vector v) {
        x_ = v.x_; y_ = v.y_;
    };
    void operator+=(Vector v) {
        x_ += v.x_; y_ += v.y_;
    };
    void operator-=(Vector v) {
        x_ -= v.x_; y_ -= v.y_;
    };
    void operator*=(double f) {
        x_ *= f; y_ *= f;
    }
    int operator==(Vector v) {
        return (x_ == v.x_ && y_ == v.y_);
    };
    Vector operator+(Vector v) {
        return Vector(x_ + v.x_, y_ + v.y_);
    };
    Vector operator-(Vector v) {
        return Vector(x_ - v.x_, y_ - v.y_);
    };
    Vector operator*(double f) {
        return Vector(x_*f, y_*f);
    }
    Vector operator/(double f) {
        return Vector(x_/f, y_/f);
    }

    double x_;
    double y_;
};

struct Node
{
    node_id_t id_;  // unique id of this node
    model_time_t startTime_;    // time started current move
    model_time_t arrivalTime_;  // time of arrival
    model_time_t nextStartTime_;// time for the next move
    Vector pos_;    // current position
    Vector dest_;   // destination
    double speed_;  // the speed of moving
    long numMoves_; // number of moves made so far
};

struct Move
{
    model_time_t time_;
    node_id_t id_;
    Vector dest_;
    double speed_;
    bool firstMove_;

    model_time_t arrivalTime_;
    model_time_t nextStartTime_;
};

enum AreaType
{
    AREA_UNKNOWN = -1,
    AREA_RECT = 0,
};

class Area
{
public:
    Area(char *areaType, long seed = 0);
    ~Area();
    void randomPos(Vector &v);
    void center(Vector &v);
    double maxX();
    double maxY();
    // is p inside this area?
    bool inside(Vector &p);
    // find the nearest point to p inside the area
    // results are in p
    // if p is already inside, p is unchanged
    void findNearestPoint(Vector &p);

protected:
    AreaType type_;
    double *params_;
    long seed_;
};

enum ModelType
{
    MODEL_UNKNOWN = -1,
    MODEL_WAYPOINT = 0,
    MODEL_REFPOINT,
    MODEL_FIXEDWP,
    MODEL_BROWNIAN,
    MODEL_PURSUE,
    MODEL_COLUMN,
    MODEL_GAUSS_MARKOV,
};

class Model
{
public:
    Model(long seed = 0);
    virtual ~Model();

    // get the type of the model
    ModelType type();

    // access to the nodes
    Node *getNode(node_id_t id);    // get one node given its id
    inline Node *getNodeList() {return nodes_;}; // get the list

    // access the parameters
    char *get(char *key);
    int geti(char *key);
    long getl(char *key);
    double getf(char *key);

    int set(char *key, char *value);
    void copyParam(Model *target);
    void dump();

    // generation of movents
    virtual void init(model_time_t startTime, \
                      model_time_t stopTime, \
                      node_id_t startID, \
                      int numNodes, \
                      Area *area, \
                      bool cp = false);

    // if probeOnly == true 
    // just check what should be the next move
    // not doing any changes to nodes_
    model_time_t nextMove(Move *move, bool probeOnly = false);

    inline void enableAcceleration() {enableAcceleration_ = true;};

protected:
    virtual void makeMove(Node *node);

    void makeAcceleratedMove(Node *node);
    void makePendingMoves(List *pList, Node *sn);

    void createShadow();
    void initNodes(Node *nodes, int numNodes);

    // find the node with the minimum next start time
    int findMinNode(model_time_t &minTime);

    void getDistType(char *typeName, DistType &dist);
    double getRandomDouble(DistType type, double param1, double param2);

    // type of the model
    ModelType type_;

    // common parameters
    model_time_t startTime_;
    model_time_t stopTime_;
    node_id_t startID_;
    int numNodes_;
    Area *area_;

    // other parameters that are unique to each model
    List *paramList_;

    // list of nodes
    Node *nodes_;

    // acceleration parameters
    bool enableAcceleration_;
    model_time_t ACC_T_;
    double ACC_A_;

    // List of shadow nodes
    Node *shadowNodes_;

    // pending moves
    List **pendingMoves_;

    bool initialized_;
    long seed_;
};

#endif
