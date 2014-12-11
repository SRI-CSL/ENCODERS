/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Yu-Ting Yu (yty)
 *   Hasnain Lakhani (HL)
 */

#include "LossRateSlidingWindowElement.h"

void LossRateSlidingWindowElement::advance(bool flag) {
    double reading = flag? 0.0 : 1.0;
    Timeval now = Timeval::now();
    double deltat = (now - lastAdvanceTime).getTimeAsSecondsDouble();
    double alpha = 1.0 - exp(deltat / (-1.0*windowSize));
    lossRate = (alpha * reading) + ((1.0-alpha) * lossRate);
    lastAdvanceTime = now;
}
