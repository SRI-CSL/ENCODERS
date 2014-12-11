//
// random.h
//
// Generation of various types of random numbers
//
// A singleton class
//

#ifndef _random_
#define _random_

//#define RANDOM_SIMPLE


// parameters used in random number generation
#ifdef RANDOM_SIMPLE
    // ran0
    #define IA 16807
    #define IM 2147483647
    #define AM (1.0/IM)
    #define IQ 127773
    #define IR 2836
    #define MASK 123459876
#else
    // ran1
    #define IM1 2147483563
    #define IM2 2147483399
    #define AM  (1.0/IM1)
    #define IMM1 (IM1-1)
    #define IA1 40014
    #define IA2 40692
    #define IQ1 53668
    #define IQ2 52774
    #define IR1 12211
    #define IR2 3791
    #define NTAB 32
    #define NDIV (1+IMM1/NTAB)
    #define EPS 1.2e-7
    #define RNMX (1.0-EPS) 
#endif

enum DistType {
    DIST_UNKNOWN = -1,
    DIST_CONST   = 0,
    DIST_UNIFORM = 1,
    DIST_GAUSS   = 2,
};

class Random
{
public:
    ~Random();
    static Random * instance(long seed = 0);
    float uniform();
    float gauss();

private:
    Random(long seed);   // singleton
    static Random * instance_;

    static long idum_;
};

inline float UNIFORM (long seed) 
{
    return Random::instance(seed)->uniform();
}

inline float GAUSS (long seed) 
{
    return Random::instance(seed)->gauss();
}

#endif
