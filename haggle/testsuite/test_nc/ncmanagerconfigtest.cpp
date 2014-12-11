/* Copyright (c) 2014 SRI International
 * Developed under DARPA contract N66001-11-C-4022.
 * Authors:
 *   Joshua Joy (JJ, jjoy)
 */

/*
 * ncmanagerconfigtest.cpp
 *
 *  Created on: Jul 18, 2012
 *      Author: jjoy
 */

#include <stdio.h>

#include "DataManager.h"
#include "NodeManager.h"
#include "ProtocolManager.h"
#include "ProtocolUDP.h"
#include "ConnectivityManager.h"
#include "ForwardingManager.h"
#include "SecurityManager.h"
#include "ApplicationManager.h"
#include "ResourceManager.h"
#include "SQLDataStore.h"
#include "DebugManager.h"
#include "../../src/hagglekernel/Trace.h"
#include "../../src/hagglekernel/networkcoding/manager/NetworkCodingManager.h"

#define TEST_SQL_DATABASE   "/tmp/test.db"

void testMap() {
    Map<string,List<string> > networkCodedBlocksType;

    List<string> found = networkCodedBlocksType["josh"];
    if( found.empty()) {
        printf("not found\n");
        found.push_back("menlopark");
        networkCodedBlocksType["josh"]  = found;
    }

    found = networkCodedBlocksType["josh"];
    printf("now isempty=|%d|\n",found.empty());

    printf("now iterating\n");


    for (List<String>::iterator it = found.begin(); it != found.end(); it++) {
        printf("item=|%s|\n",(*it).c_str());
    }


}

void debug() {
    string configFile = "";
    DataStore *ds = new SQLDataStore(true, TEST_SQL_DATABASE);
    const string storagepath = HAGGLE_DEFAULT_STORAGE_PATH;

    HaggleKernel *kernel = NULL;
    kernel = new HaggleKernel(configFile,ds,storagepath);

    ApplicationManager *am = NULL;
    DataManager *dm = NULL;
    NodeManager *nm = NULL;
    ProtocolManager *pm = NULL;
    ForwardingManager *fm = NULL;
    SecurityManager *sm = NULL;
    ConnectivityManager *cm = NULL;
    ResourceManager *rm = NULL;
    NetworkCodingManager* networkCodingManager = NULL;
    DebugManager *db = NULL;

    bool initialized = false;

    initialized = kernel->init();

    am = new ApplicationManager(kernel);
    initialized =am->init();

    dm = new DataManager(kernel);
    initialized =dm->init();

    nm = new NodeManager(kernel);
    initialized = nm->init();

    pm = new ProtocolManager(kernel);
    initialized = pm->init();

    fm = new ForwardingManager(kernel);
    initialized = fm->init();

    sm = new SecurityManager(kernel);
    initialized = sm->init();

    networkCodingManager = new NetworkCodingManager(kernel);
    initialized = networkCodingManager->init();


    db = new DebugManager(kernel, true);
    initialized = db->init();



    kernel->run();

    while (true) {
        sleep(5);
    }

}

int main(int argc, char *argv[]) {
    printf("network coding manager test\n");

    testMap();
//    debug();

    return 0;
}
