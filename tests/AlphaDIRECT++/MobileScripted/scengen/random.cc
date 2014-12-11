//
// random.cc
//
// Generation of various types of random numbers
//

#include "defs.h"

#include <math.h>
#include <time.h>
#include "random.h"

Random *Random::instance_ = NULL;
long Random::idum_ = -1;

// Constructor
Random::Random(long seed)
{
    if (seed == 0) {    // give another seed
        // the seed. should be a negative number
        idum_ = -1 * time(NULL);
    } else {
        idum_ = - labs(seed);
    }
    // printf("Setting seed to %ld\n", idum_);
}

// Destructor
Random::~Random()
{
}

// This is a singleton
Random* Random::instance(long seed)
{
    if (instance_ == NULL) {
        instance_ = new Random(seed);
        assert (instance_ != NULL);
    }
    return instance_;
}

// The uniform random number
// returns a float that uniformly distributed over (0,1)
float Random::uniform()
{
#ifdef RANDOM_SIMPLE
    //printf("use Simple random\n");

    long k;
    float ans;

    idum_ ^= MASK;
    k = idum_ / IQ;
    idum_ = IA * (idum_ - k*IQ) - IR * k;
    if (idum_ < 0)
        idum_ += IM;
    ans = AM * idum_;
    idum_ ^= MASK;
    return ans;
#else
    //printf("use Complex random\n");

    int j;
    long k;
    static long idum2 = 123456789;
    static long iy = 0;
    static long iv[NTAB];
    float temp;

    if (idum_<=0) {                      //initialize
        if(-(idum_)<1) idum_ = 1;         //Be sure to prevent idum_ = 0
        else idum_ = -(idum_);
        idum2 = idum_;
        for (j=NTAB+7;j>=0;j--){          //load the shuffle table 
            k = (idum_)/IQ1;                //after 8 warm-ups 
            idum_ = IA1*(idum_ - k*IQ1) - k*IR1;
            if(idum_<0) idum_+=IM1;
            if(j<NTAB) iv[j] = idum_;
        } // for
        iy = iv[0];
    } // if

    k = (idum_)/IQ1;                    //start here when not initializing
    idum_ = IA1*(idum_ - k*IQ1) - k*IR1;//compute idum_=(IA1idum_)%IM1 without
    if (idum_<0) idum_+=IM1;            //overflows by Schrage Method
    k = idum2/IQ2;
    idum2 = IA2*(idum2 - k*IQ2) - k*IR2;//compute idum2=(IA2*idum2)%IM2 likewise
    if (idum2<0) idum2+=IM2;
    j = iy/NDIV;                        //will be in range 0...NTAB - 1
    iy = iv[j] - idum2;                 //Here idum is shuffled, idum_ and idum2
    iv[j] = idum_;                      //are combined to generate the output
    if (iy<1) iy+=IMM1;
    if ((temp = AM*iy)>RNMX) return RNMX; //users do not expect end point values
    else return temp;
#endif
}

// This method generates a zero mean, unit variance gaussian random var
float Random::gauss()
{
    static int iset = 0;
    static float gset;
    float fac,rsq,v1,v2;
  
    if (idum_<0) iset = 0;
    if (iset==0) {
        do {
            v1 = 2.0*uniform()-1.0;
            v2 = 2.0*uniform()-1.0;
            rsq = v1*v1+v2*v2;
        } while (rsq>=1.0||rsq==0.0);

        fac = sqrt(-2.0*log(rsq)/rsq);
        gset = v1*fac;
        iset = 1;
        return v2*fac;
    } 
    else 
    {
        iset = 0;
        return gset;
    }
}
