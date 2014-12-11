/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Yu-Ting Yu (yty)
 *   Hasnain Lakhani (HL)
 */


#ifndef _LOSSESTIMATEMANAGER_H
#define _LOSSESTIMATEMANAGER_H

#include <libcpphaggle/List.h>
#include <libcpphaggle/Map.h>
#include <libcpphaggle/Pair.h>
#include <libcpphaggle/Timeval.h>

using namespace haggle;

#include "Manager.h"
#include "Trace.h"
//#include <map>
#include "ManagerModule.h"
#include "LossRateSlidingWindowEstimator.h"
/*
	Return values for reporting and checking interfaces managed by
	the connectivity manager.
*/
class LossEstimateManager;

/** Loss Estimate manager.

	The purpose of the loss estimate manager estimates the loss rate of links to neighbors

	*/
class LossEstimateManager : public Manager
{
	static int tmpCount; //exists for testing, should be removed later

    // This mutex protects the known interface registry
    Mutex flagMutex; //protects flagmap

    bool init_derived();

protected:

    EventCallback <EventHandler> *periodicLossEstimateCallBack;
    EventType periodicLossEstimateEventType;
    Event *periodicLossEstimateEvent;

    void onConfig(Metadata *m);
    void lossEstimate();

    long periodicLossEstimateInterval;
	double lossRateThres;
    double initialLossRate; // CBMEN, HL
    double windowSize; // CBMEN, HL

	Map<string, bool> flagMap;
	LossRateSlidingWindowEstimator lossEstimator;

public:
    void onPeriodicLossEstimate(Event *e);
    void onIncomingBeacon(Event *e);


	double getLossRate(string iface);

    void onStartup();
    void onPrepareShutdown();
    LossEstimateManager(HaggleKernel *_kernel = haggleKernel);
    ~LossEstimateManager();
};

#endif /* _LOSSESTIMATEMANAGER_H */
