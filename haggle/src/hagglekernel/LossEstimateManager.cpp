/* Copyright (c) 2014 SRI International and GPC
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Yu-Ting Yu (yty)
 *   Joshua Joy (JJ, jjoy)
 *   Hasnain Lakhani (HL)
 */


#include "Utility.h"
#include <utils.h>

#include "SocketWrapperUDP.h"
#include "LossEstimateManager.h"

int LossEstimateManager::tmpCount = 0;

LossEstimateManager::LossEstimateManager(HaggleKernel * _haggle) :
    Manager("LossEstimateManager", _haggle),
    periodicLossEstimateEventType(-1), periodicLossEstimateEvent(NULL), periodicLossEstimateCallBack(NULL),
    periodicLossEstimateInterval(-1), lossRateThres(0.2), initialLossRate(1.0), windowSize(30.0),
    lossEstimator(1.0, 30.0)
{
}

double LossEstimateManager::getLossRate(string iface){ //return -1 if the interface doesn't exist
    return lossEstimator.queryLossRate(iface);
}

void LossEstimateManager::lossEstimate(){
   HAGGLE_DBG("loss estimate\n");
   LossEstimateManager::tmpCount++;

   Mutex::AutoLocker l(flagMutex);

   //update sliding window
   for(Map<string, bool>::iterator it = flagMap.begin();it!=flagMap.end();it++){
   	   HAGGLE_DBG("iface %s, %d\n",it->first.c_str(), it->second);
   }

    lossEstimator.advance(flagMap);
    flagMap.clear();

    NodeRefList nbrs;

	NodeRefList nbrsToDisable;
	NodeRefList nbrsToEnable;

    int numNbrs = getKernel()->getNodeStore()->retrieveNeighbors(nbrs);

    if (numNbrs <= 0) {
        return;
    }

    for (NodeRefList::iterator it = nbrs.begin(); it != nbrs.end();++it) {
        NodeRef& nbr = *it;
        NodeRef actualNbr = getKernel()->getNodeStore()->retrieve(nbr, true);
        if (!actualNbr) {
            HAGGLE_ERR("%s Actual neighbor not in node store\n", getName());
            continue;
        }

		if(nbr->isLocalDevice()) continue;

		bool aboveThreshold = false;
        bool hasLossRate = false;

		for (InterfaceRefList::const_iterator it2 = actualNbr->getInterfaces()->begin(); it2 != actualNbr->getInterfaces()->end(); it2++) {
			const InterfaceRef& iface = *it2;
			string ifacename = Interface::idString(iface);

            HAGGLE_DBG("Target node %s has loss rate %f, current tmpCount: %d \n", actualNbr->getName().c_str(), lossEstimator.queryLossRate(ifacename), LossEstimateManager::tmpCount);

            if(lossEstimator.hasLossRate(ifacename)){
                hasLossRate = true;
                if (lossEstimator.queryLossRate(ifacename) >= lossRateThres) {
                    HAGGLE_DBG("%s loss rate > threshold\n", ifacename.c_str());
                    aboveThreshold = true;
                }
            }
		}

		if(hasLossRate && aboveThreshold){
			nbrsToEnable.push_back(actualNbr);
		}
		else if (hasLossRate) {
			nbrsToDisable.push_back(actualNbr);
		}
    }

	if(nbrsToEnable.size()>0){
		HAGGLE_DBG("nbrstoenable size = %d, tmpCount = %d, send NC enable event\n", nbrsToEnable.size(), LossEstimateManager::tmpCount);
   		kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_ENABLE_NETWORKCODING, (DataObjectRef) NULL, nbrsToEnable));
    }

 	if(nbrsToDisable.size()>0){
		HAGGLE_DBG("nbrstodisable size = %d, send NC disable event\n", nbrsToDisable.size());
   		kernel->addEvent(new Event(EVENT_TYPE_DATAOBJECT_DISABLE_NETWORKCODING, (DataObjectRef) NULL, nbrsToDisable));
    }

}

void LossEstimateManager::onPeriodicLossEstimate(Event *e){
        if (getState() > MANAGER_STATE_RUNNING) {
		HAGGLE_DBG("In shutdown, ignoring periodic loss estimate\n");
		return;
	}

	HAGGLE_DBG("Periodic loss estimate\n");

	if (kernel->isShuttingDown()) {
		HAGGLE_DBG("In shutdown, ignoring event\n");
		return;
	}

        //send node description
	lossEstimate();

	if (!periodicLossEstimateEvent->isScheduled() &&
	    periodicLossEstimateInterval > 0) {
		periodicLossEstimateEvent->setTimeout(periodicLossEstimateInterval);
		kernel->addEvent(periodicLossEstimateEvent);
	}
}

bool LossEstimateManager::init_derived()
{
    int ret;
#define __CLASS__ LossEstimateManager


    periodicLossEstimateCallBack = newEventCallback(onPeriodicLossEstimate);
    periodicLossEstimateEventType = registerEventType("Periodic Loss Estimate Event",
						       onPeriodicLossEstimate);

	if (periodicLossEstimateEventType < 0) {
	  HAGGLE_ERR("Could not register periodic loss estimate event type...\n");
	  return false;
	}


	periodicLossEstimateEvent = new Event(periodicLossEstimateEventType);

	if (!periodicLossEstimateEvent)
	  return false;


	periodicLossEstimateEvent->setAutoDelete(false);
	if(periodicLossEstimateInterval > 0) {
	  periodicLossEstimateEvent->setTimeout(periodicLossEstimateInterval+periodicLossEstimateInterval/2);
	  kernel->addEvent(periodicLossEstimateEvent);
	  HAGGLE_DBG("loss estimate event added\n");
	}

    ret = setEventHandler(EVENT_TYPE_RECEIVE_BEACON, onIncomingBeacon);
    if (ret < 0) {
        HAGGLE_ERR("Could not register event handler EVENT_TYPE_RECEIVE_BEACON,     - result=%d\n", ret);
        return false;
    }


    return true;
}

/*
	Defer the startup of local connectivity until every other manager is
	prepared for startup. This will avoid any discovery of other nodes
	that might trigger events before other managers are prepared.
 */
void LossEstimateManager::onStartup()
{
    HAGGLE_DBG("Loss estimate startup\n");

}

void LossEstimateManager::onPrepareShutdown()
{
    signalIsReadyForShutdown();
}

LossEstimateManager::~LossEstimateManager()
{
    if (periodicLossEstimateEvent) {
	  if (periodicLossEstimateEvent->isScheduled())
	    periodicLossEstimateEvent->setAutoDelete(true);
	  else
	    delete periodicLossEstimateEvent;
	}

    if (periodicLossEstimateCallBack) {
        delete periodicLossEstimateCallBack;
    }

	Event::unregisterType(periodicLossEstimateEventType);

}

void LossEstimateManager::onConfig(Metadata *m)
{
	Metadata *nm = m->getMetadata("PeriodicLossEstimate");

	if (nm) {
		char *endptr = NULL;
		const char *param = nm->getParameter("interval");

		if (param) {
			unsigned long interval = strtoul(param, &endptr, 10);

			if (endptr && endptr != param) {
				HAGGLE_DBG("Setting PeriodicLossEstaimte interval to %lu\n", interval);
//				kernel->getThisNode()->setMatchingThreshold(matchingThreshold);
				periodicLossEstimateInterval = interval;
				LOG_ADD("# %s: interval=%lu\n", getName(), interval);

                if (interval > 0) {
                    HAGGLE_DBG("loss estimate event added\n");
                    periodicLossEstimateEvent->setTimeout(periodicLossEstimateInterval+periodicLossEstimateInterval/2);
                    kernel->addEvent(periodicLossEstimateEvent);
                }
			}
		}
	}

	nm = NULL;
	nm = m->getMetadata("NetworkCodingTrigger");

	if (nm) {
		char *endptr = NULL;
		const char *param = nm->getParameter("loss_rate_threshold");

		if (param) {
			double thres = strtod(param, &endptr);

			if (endptr && endptr != param) {
				HAGGLE_DBG("Setting Loss Rate Threshold to %f\n", thres);
//				kernel->getThisNode()->setMatchingThreshold(matchingThreshold);
				lossRateThres = thres;
				LOG_ADD("# %s: loss_rate_threshold=%f\n", getName(), thres);
			}
		}

        // CBMEN, HL, Begin
        param = nm->getParameter("initial_loss_rate");
        if (param) {
            double tmp = strtod(param, &endptr);

            if (endptr && endptr != param) {
                HAGGLE_DBG("Setting Initial Loss Rate to %f\n", tmp);
                initialLossRate = tmp;
            }
        }

        param = nm->getParameter("window_size");
        if (param) {
            double tmp = strtod(param, &endptr);

            if (endptr && endptr != param) {
                HAGGLE_DBG("Setting Window Size to %f\n", tmp);
                windowSize = tmp;
            }
        }
        // CBMEN, HL, End
	}

    lossEstimator = LossRateSlidingWindowEstimator(initialLossRate, windowSize);

}

void LossEstimateManager::onIncomingBeacon(Event *e){

    HAGGLE_DBG("BEACON RECEIVED\n");

    const InterfaceRef& iface = e->getInterface();

     if(!iface->isLocal() &&
           !iface->isApplication()
           ){
            HAGGLE_DBG("Received ethernet beacon from remote interface %s\n", iface->getIdentifierStr());
            string ifacename = Interface::idString(iface);

            Mutex::AutoLocker l(flagMutex);
            this->flagMap[ifacename] = true;
        }
}
