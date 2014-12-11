/* Copyright 2008 Uppsala University
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 *     
 *     http://www.apache.org/licenses/LICENSE-2.0 
 *
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License.
 */ 

/*
	This is a haggle main function. It's purpose is to let the testsuite run
	a haggle kernel without the need to start a separate program that is haggle.
*/

#include "hagglemain.h"

#include <libcpphaggle/Thread.h>
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
#include <libcpphaggle/Exception.h>

using namespace haggle;

#if defined(OS_WINDOWS)
#define TEST_SQL_DATABASE	"test.db"
#else
#define TEST_SQL_DATABASE	"/tmp/test.db"
#endif

class hidden_hagglemainRunnable;

static hidden_hagglemainRunnable	*current_haggle = NULL;
HaggleKernel *kernel = NULL;
static bool shutdown_started = false;

static Condition *signalling_condition;

class hidden_hagglemainRunnable : public Runnable {
	long flags;
public:
	hidden_hagglemainRunnable(long _flags) : flags(_flags) {}
	~hidden_hagglemainRunnable() {}

	bool run()
	{
		ApplicationManager *am = NULL;
		DataManager *dm = NULL;
		NodeManager *nm = NULL;
		ProtocolManager *pm = NULL;
		ForwardingManager *fm = NULL;
		SecurityManager *sm = NULL;
		ConnectivityManager *cm = NULL;
		ResourceManager *rm = NULL;
		
		/* Seed the random number generator */
		srand(Timeval::now().getMicroSeconds());

		try{

	        string configFile = "";
	        DataStore *ds = new SQLDataStore(true, TEST_SQL_DATABASE);
	        const string storagepath = HAGGLE_DEFAULT_STORAGE_PATH;

			kernel = new HaggleKernel(configFile,ds,storagepath);
		}catch (Exception&){
			kernel = NULL;
		}
		
		if(!kernel)
		{
			return false;
		}
		
		// Build a Haggle configuration
		try{
			ProtocolSocket *p = NULL;
			
			if(flags & hagglemain_have_application_manager)
			{
				am = new ApplicationManager(kernel);
			}

			if(flags & hagglemain_have_data_manager)
			{
				dm = new DataManager(kernel);
			}
			
			if(flags & hagglemain_have_node_manager)
			{
				nm = new NodeManager(kernel);
			}
			
			if(flags & hagglemain_have_protocol_manager)
			{
				pm = new ProtocolManager(kernel);
			}
			
			if(flags & hagglemain_have_forwarding_manager)
			{
				fm = new ForwardingManager(kernel);
			}
			
			if(flags & hagglemain_have_security_manager)
			{
				sm = new SecurityManager(kernel);
			}
			
			if(flags & hagglemain_have_protocol_manager)
			{
				p = new ProtocolUDP("127.0.0.1", HAGGLE_SERVICE_DEFAULT_PORT, pm);
				p->setFlag(PROT_FLAG_APPLICATION);
				p->registerWithManager();
			}
			
			if(flags & hagglemain_have_resource_manager)
			{
				rm = new ResourceManager(kernel);
			}
			
			/* Add ConnectivityManager last since it will start to
			 * discover interfaces and generate events. At that
			 * point the other managers should already be
			 * running. */
			if(flags & hagglemain_have_connectivity_manager)
			{
				cm = new ConnectivityManager(kernel);
			}
		}catch (Exception&){
			goto fail_exception;
		} 
		
		try {
			HAGGLE_DBG("Starting kernel...\n");
			if (signalling_condition != NULL)
				signalling_condition->signal();
			kernel->run();
			HAGGLE_DBG("Haggle finished\n");
		} catch (Exception&)
		{

		}
		
fail_exception:
		if(cm)
			delete cm;
		if(sm)
			delete sm;
		if(fm)
			delete fm;
		if(pm)
			delete pm;
		if(nm)
			delete nm;
		if(dm)
			delete dm;
		if(am)
			delete am;
		if(kernel)
			delete kernel;
		kernel = NULL;

		return false;
	}
	
	void cleanup()
	{
		current_haggle = NULL;
		delete this;
	}
};

bool hagglemain_start(long flags, Condition *cond)
{
	if(current_haggle != NULL)
		return false;
	
	signalling_condition = cond;
	
	try{
		current_haggle = new hidden_hagglemainRunnable(flags);
	}catch(Exception &){
		goto fail_current_haggle;
	}
	try{
		if(current_haggle->start() != 0)
			goto fail_thread;
	}catch(Exception &){
		goto fail_thread;
	}
	shutdown_started = false;
	return true;
fail_thread:
	delete current_haggle;
	current_haggle = NULL;
fail_current_haggle:
	return false;
}

bool hagglemain_is_running(void)
{
	return (current_haggle != NULL);
}

bool hagglemain_stop(void)
{
	if(shutdown_started)
		return false;
	
	if(current_haggle == NULL)
		return false;
	
	if(kernel == NULL)
		return false;
	
	kernel->shutdown();
	shutdown_started = true;
	return true;
}

bool hagglemain_stop_and_wait(void)
{
	if(hagglemain_stop())
	{
		current_haggle->join();
		return true;
	}
	return false;
}

