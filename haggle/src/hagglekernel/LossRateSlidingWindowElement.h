/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Yu-Ting Yu (yty)
 *   Hasnain Lakhani (HL)
 */

#ifndef _LOSSRATESLIDINGWINDOWELEMENT_H
#define _LOSSRATESLIDINGWINDOWELEMENT_H

#include <math.h>
#include <libcpphaggle/List.h>
#include <libcpphaggle/Map.h>
#include <libcpphaggle/Pair.h>
#include <libcpphaggle/Timeval.h>

using namespace haggle;

#include "Manager.h"
#include "Trace.h"
class LossRateSlidingWindowElement;

class LossRateSlidingWindowElement {
private:
    double lossRate;
    Timeval lastAdvanceTime;
    double windowSize;
public:
    LossRateSlidingWindowElement(double initialLossRate, double _windowSize): lossRate(initialLossRate), lastAdvanceTime(Timeval::now()), windowSize(_windowSize) {}
    LossRateSlidingWindowElement(LossRateSlidingWindowElement const &l): lossRate(l.lossRate), lastAdvanceTime(l.lastAdvanceTime), windowSize(l.windowSize) {}
    virtual LossRateSlidingWindowElement& operator=(LossRateSlidingWindowElement const & l) {
        lossRate = l.lossRate;
        lastAdvanceTime = l.lastAdvanceTime;
        windowSize = l.windowSize;
        return *this;
    }
    void advance(bool flag);
    double getLossRate() { return lossRate; }
    double getWindowSize() { return windowSize; }
};

#endif /* _LOSSRATESLIDINGWINDOWELEMENT_H */
